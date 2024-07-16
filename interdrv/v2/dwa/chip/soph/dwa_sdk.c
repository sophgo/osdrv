
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <uapi/linux/sched/types.h>

#include <linux/comm_video.h>
#include <linux/comm_gdc.h>
#include <linux/dwa_uapi.h>

#include <base_cb.h>
#include <vb.h>
#include <sys.h>
#include "dwa_debug.h"
#include "dwa_core.h"
#include "dwa_sdk.h"
#include "dwa_common.h"
#include "dwa.h"
#include "cmdq.h"
#include "ion.h"
#include "vbq.h"
#include "dwa_proc.h"
#include "mesh.h"

#define YUV_8BIT(y, u, v) ((((y)&0xff) << 16) | (((u)&0xff) << 8) | ((v)&0xff))
#define BGCOLOR_GRAY  (0x808080)
#define BGCOLOR_GREEN  (YUV_8BIT(0, 128, 128))
#define DWA_INTR_EN_ALL (0x07)
#define DWA_INTR_EN_NULL (0x0)
#define DWA_INVALID_CORE_ID (DWA_DEV_MAX_CNT + 1)

#define USE_EXCEPTION_HDL 0
#define USE_REAL_CMDQ 0

static unsigned int bgcolor = BGCOLOR_GREEN;
static unsigned int border_color = BGCOLOR_GREEN;
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

int dwa_exec_cb(void *dev, enum enum_modules_id caller, unsigned int cmd, void *arg)
{
	struct dwa_vdev *wdev = (struct dwa_vdev *)dev;
	struct mesh_dwa_cfg *cfg;
	mod_id_e mod_id;
	int rc = -1;

	switch (cmd) {
		case DWA_CB_MESH_GDC_OP: {//LDC_CB_MESH_GDC_OP cfg by internal MOD
			mutex_lock(&g_mesh_lock);
			cfg = (struct mesh_dwa_cfg *)arg;//struct mesh_dwa_cfg cfg by internal MOD
			mod_id = convert_mod_id(caller);
			mutex_unlock(&g_mesh_lock);

			if (mod_id == ID_BUTT) {
				TRACE_DWA(DBG_WARN, "invalid mod do GDC_OP\n");
				return -1;
			}
			rc = mesh_dwa_do_op(wdev, cfg->usage, cfg->usage_param
				, cfg->vb_in, cfg->pix_format, cfg->mesh_addr
				,cfg->sync_io, cfg->cb_param, cfg->cb_param_size
				, mod_id, cfg->rotation);
			break;
		}
		default: {
			TRACE_DWA(DBG_WARN, "invalid cb CMD\n");
			break;
		}
	}

	return rc;
}

int dwa_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_DWA);
}

int dwa_reg_cb(struct dwa_vdev *wdev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_DWA;
	reg_cb.dev		= (void *)wdev;
	reg_cb.cb		= dwa_exec_cb;

	return base_reg_module_cb(&reg_cb);
}

static void dwa_op_done_cb(mod_id_e mod_id, void *param, vb_blk blk)
{
	struct dwa_op_done_cfg cfg;
	struct base_exe_m_cb exe_cb;
	enum enum_modules_id callee = convert_cb_id(mod_id);

	TRACE_DWA(DBG_DEBUG, "callee=%d, mod_id(%d)\n", callee, mod_id);
	cfg.param = param;
	cfg.blk = blk;

	exe_cb.callee = callee;
	exe_cb.caller = E_MODULE_DWA;
	exe_cb.cmd_id = DWA_CB_DWA_OP_DONE;
	exe_cb.data   = &cfg;
	base_exe_module_cb(&exe_cb);
}

static void dwa_hdl_hw_tsk_cb(struct dwa_vdev *wdev, struct dwa_job *job
	, struct dwa_task *tsk, bool is_last)
{
	bool is_internal;
	vb_blk blk_in = VB_INVALID_HANDLE, blk_out = VB_INVALID_HANDLE;
	mod_id_e mod_id;
	unsigned char isLatask;
	struct dwa_vb_done *vb_done = NULL;
	unsigned long flags;

	if (unlikely(!wdev || !job || !tsk)) {
		TRACE_DWA(DBG_ERR, "invalid param\n");
		return;
	}
	is_internal = (tsk->attr.reserved == DWA_MAGIC);
	mod_id = job->identity.mod_id;

	TRACE_DWA(DBG_DEBUG, "is_internal=%d\n", is_internal);

	/* []internal module]:
	 *  sync_io or async_io
	 *  release imgin vb_blk at task done.
	 *  mesh prepared by module.
	 *
	 * []user case]:
	 *  always sync_io.
	 *  Don't care imgin/imgout vb_blk. User release by themselves.
	 *  mesh prepared by dwa at AddXXXTask.
	 *
	 */
	if (is_internal) {
		// for internal module handshaking. such as vi/vpss rotation.
		isLatask = (u8)tsk->attr.private_data[1];

		TRACE_DWA(DBG_DEBUG, "isLatask=%d, blk_in(pa=0x%llx), blk_out(pa=0x%llx)\n",
				isLatask,
				(unsigned long long)tsk->attr.img_in.video_frame.phyaddr[0],
				(unsigned long long)tsk->attr.img_out.video_frame.phyaddr[0]);

		blk_in = vb_phys_addr2handle(tsk->attr.img_in.video_frame.phyaddr[0]);
		blk_out = vb_phys_addr2handle(tsk->attr.img_out.video_frame.phyaddr[0]);
		if (blk_out == VB_INVALID_HANDLE)
			TRACE_DWA(DBG_ERR, "blk_out is invalid vb_blk, no callback to(%d)\n", mod_id);
		else {
			atomic_long_fetch_and(~BIT(ID_DWA), &((struct vb_s *)blk_out)->mod_ids);
			if (isLatask)
				dwa_op_done_cb(mod_id, (void *)(uintptr_t)tsk->attr.private_data[2], blk_out);
		}

		// User space:
		//   Caller always assign callback.
		//   Null callback used for internal dwa sub job.
		// Kernel space:
		//  !isLatask used for internal dwa sub job.
		if (isLatask && blk_in != VB_INVALID_HANDLE) {
			atomic_long_fetch_and(~BIT(ID_DWA), &((struct vb_s *)blk_in)->mod_ids);
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

			TRACE_DWA(DBG_DEBUG, "sem.count[%d]\n", wdev->vb_doneq.sem.count);
			up(&wdev->vb_doneq.sem);
			TRACE_DWA(DBG_DEBUG, "sem.count[%d]\n", wdev->vb_doneq.sem.count);

			TRACE_DWA(DBG_DEBUG, "vb_done->img_out[%llx-%d]\n"
				, vb_done->img_out.video_frame.phyaddr[0], vb_done->img_out.video_frame.width);
			TRACE_DWA(DBG_DEBUG, "vb_doneq identity[%d-%d-%s]\n"
				, vb_done->job.identity.mod_id, vb_done->job.identity.id, vb_done->job.identity.name);
		}
	}
}

static void dwa_wkup_cmdq_tsk(struct dwa_core *core) {
#if USE_REAL_CMDQ
	(void)(core);
#else
	TRACE_DWA(DBG_DEBUG, "dwa_wkup_cmdq_tsk\n");

	core->cmdq_evt = true;
	wake_up_interruptible(&core->cmdq_wq);
#endif
}

static void dwa_notify_wkup_evt_kth(void *data, enum dwa_wait_evt evt)
{
	struct dwa_vdev *dev = (struct dwa_vdev *)data;
	unsigned long flags;

	if(!dev) {
		TRACE_DWA(DBG_ERR, "dwa vdev isn't created yet.\n");
		return;
	}

	spin_lock_irqsave(&dev->job_lock, flags);
	dev->evt |= evt;
	TRACE_DWA(DBG_DEBUG, "evt[%d], dev evt[%d]\n", evt, dev->evt);
	spin_unlock_irqrestore(&dev->job_lock, flags);

	wake_up_interruptible(&dev->wait);
}

static void dwa_clr_evt_kth(void *data)
{
	struct dwa_vdev *dev = (struct dwa_vdev *)data;
	unsigned long flags;
	enum dwa_wait_evt evt;

	if(!dev) {
		TRACE_DWA(DBG_ERR, "ldc vdev isn't created yet.\n");
		return;
	}

	spin_lock_irqsave(&dev->job_lock, flags);
	evt = dev->evt;
	dev->evt &= ~evt;
	TRACE_DWA(DBG_DEBUG, "evt[%d], dev evt[%d]\n", evt, dev->evt);
	spin_unlock_irqrestore(&dev->job_lock, flags);
}

static void dwa_work_handle_job_done(struct dwa_vdev *dev, struct dwa_job *job, unsigned char irq_coreid)
{
	unsigned long flags;
	struct fasync_struct *fasync;

	if (!job) {
		TRACE_DWA(DBG_ERR, "null job\n");
		return;
	}

	atomic_set(&job->job_state, DWA_JOB_END);

	dwa_proc_record_job_done(job);

	spin_lock_irqsave(&dev->job_lock, flags);
	dev->job_cnt--;
	spin_unlock_irqrestore(&dev->job_lock, flags);
	dwa_notify_wkup_evt_kth(dev, DWA_EVENT_EOF);

	if (job->identity.sync_io) {
		TRACE_DWA(DBG_INFO, "job[%px]\n", job);
		job->job_done_evt = true;
		wake_up(&job->job_done_wq);
	} else {
		kfree(job);
		job = NULL;
		fasync = dwa_get_dev_fasync();
		kill_fasync(&fasync, SIGIO, POLL_IN);
	}
	TRACE_DWA(DBG_DEBUG, "job done\n");
}

static void dwa_work_handle_tsk_done(struct dwa_vdev *dev
	, struct dwa_task *tsk, struct dwa_job *job, unsigned char irq_coreid, bool is_last_tsk)
{
	unsigned long flags;

	if (!tsk) {
		TRACE_DWA(DBG_ERR, "null tsk\n");
		return;
	}

	if (atomic_read(&tsk->state) == DWA_TASK_STATE_RUNNING) {
		atomic_set(&tsk->state, DWA_TASK_STATE_DONE);
		atomic_dec(&job->task_num);

		dwa_hdl_hw_tsk_cb(dev, job, tsk, is_last_tsk);

		dwa_proc_record_hw_tsk_done(job, tsk);

		spin_lock_irqsave(&dev->job_lock, flags);
		TRACE_DWA(DBG_DEBUG, "tsk[%px]\n", tsk);
		kfree(tsk);
		tsk = NULL;
		spin_unlock_irqrestore(&dev->job_lock, flags);

		TRACE_DWA(DBG_DEBUG, "tsk done, is last tsk[%d]\n", is_last_tsk);
		if (is_last_tsk) {
			dwa_work_handle_job_done(dev, job, irq_coreid);
		}
	} else {
		TRACE_DWA(DBG_ERR, "invalid tsk state(%d).\n"
			, (enum dwa_task_state)atomic_read(&tsk->state));
	}
}

void dwa_work_handle_frm_done(struct dwa_vdev *dev, struct dwa_core *core)
{
	struct dwa_job *job;
	struct dwa_task *tsk;
	unsigned long flags;
	unsigned char irq_coreid;
	bool is_last_tsk = false;

	if(!dev) {
		TRACE_DWA(DBG_ERR, "dwa vdev isn't created yet.\n");
		return;
	}

	atomic_set(&core->state, DWA_CORE_STATE_IDLE);
	irq_coreid = (u8)core->dev_type;
	TRACE_DWA(DBG_INFO, "core(%d) state set(%d)\n", irq_coreid, DWA_CORE_STATE_IDLE);

	if (unlikely(irq_coreid) >= DEV_DWA_MAX)
		return;

	spin_lock_irqsave(&dev->job_lock, flags);
	job = list_first_entry_or_null(&dev->list.done_list[irq_coreid], struct dwa_job, node);
	spin_unlock_irqrestore(&dev->job_lock, flags);

	if (unlikely(!job))
		return;

	if (atomic_read(&job->job_state) == DWA_JOB_WORKING) {
		spin_lock_irqsave(&dev->job_lock, flags);
		tsk = list_first_entry_or_null(&dev->core[irq_coreid].list.done_list, struct dwa_task, node);
		if (unlikely(!tsk)) {
			spin_unlock_irqrestore(&dev->job_lock, flags);
			return;
		}
		is_last_tsk = (atomic_read(&job->task_num) > 1 ? false : true);
		list_del(&tsk->node);
		list_del(&job->node);
		spin_unlock_irqrestore(&dev->job_lock, flags);

		dwa_work_handle_tsk_done(dev, tsk, job, irq_coreid, is_last_tsk);

		if (job && job->use_cmdq && !is_last_tsk)
			dwa_wkup_cmdq_tsk(&dev->core[irq_coreid]);
	} else {
		TRACE_DWA(DBG_ERR, "invalid job(%px) state(%d).\n"
			, job, (enum dwa_job_state)atomic_read(&job->job_state));
	}
}

#if DWA_USE_WORKQUEUE
static void dwa_work_frm_done(struct work_struct *work)//intr post handle
{
	struct dwa_core *core = container_of(work, struct dwa_core, work_frm_done);
	struct dwa_vdev *dev = dwa_get_dev();

	if(!core || !dev) {
		TRACE_DWA(DBG_ERR, "dwa vdev or core isn't created yet.\n");
		return;
	}

	dwa_work_handle_frm_done(dev, core);
}
#endif

static void dwa_wkup_frm_done_work(void *data, int top_id)
{
	struct dwa_core *core = (struct dwa_core *)data;
	(void)top_id;

	if (!core) {
		TRACE_DWA(DBG_ERR, "dwa core isn't created yet.\n");
		return;
	}

#if DWA_USE_WORKQUEUE
	//queue_work(dev->workqueue, &dev->work_frm_done);//wakeup post handle
	schedule_work(&core->work_frm_done);
#else
	dwa_work_handle_frm_done(dwa_get_dev(), core);
#endif
}

static void dwa_reset_cur_abord_tsk(struct dwa_vdev *dev, unsigned char coreid, struct dwa_job *job)
{
#if USE_EXCEPTION_HDL
	struct dwa_task *tsk, tsk_tmp;

	if (unlikely(!dev)) {
		TRACE_DWA(DBG_ERR, "dwa vdev isn't created yet.\n");
		return;
	}

	if (unlikely(!job)) {
		TRACE_DWA(DBG_ERR, "err job is null\n");
		return;
	}

	if (list_empty(&job->task_list)) {
		TRACE_DWA(DBG_ERR, "err tsklist is null\n");
		return;
	}

	list_for_each_entry_safe(tsk, tsk_tmp, &job->task_list, node) {
		if (unlikely(!tsk)) {
			TRACE_DWA(DBG_ERR, "cur job(%px) cur tsk is null\n", job);
			continue;
		}

		if (unlikely(tsk->tsk_id < 0 || tsk->tsk_id >= DWA_JOB_MAX_TSK_NUM)) {
			TRACE_DWA(DBG_ERR, "cur job(%px) cur tsk id(%d) invalid\n", job, tsk->tsk_id);
			continue;
		}

		dwa_intr_ctrl(DWA_INTR_EN_NULL, coreid);
		dwa_disable(coreid);
		dwa_reset(coreid);

		atomic_set(&dev->core[coreid]->state, DWA_CORE_STATE_IDLE);

		dwa_notify_wkup_evt_kth(dev, DWA_EVENT_RST);
		up(&tsk->sem);
	}

	atomic_set(&dev->state, DWA_DEV_STATE_RUNNING);
#else
	(void) dev;
	(void) coreid;
	(void) job;
	return;
#endif
}

static void dwa_try_reset_abort_job(struct dwa_vdev *wdev)
{
	unsigned char coreid;
	unsigned long flags;
	struct dwa_job *work_job;

	spin_lock_irqsave(&wdev->job_lock, flags);
	for (coreid = 0; coreid < wdev->core_num; coreid++) {
		work_job = list_first_entry_or_null(&wdev->list.work_list[coreid], struct dwa_job, node);
		if (unlikely(!work_job)) {
			TRACE_DWA(DBG_NOTICE, "dwa vdev core[%d] select timeout,hw busy\n", coreid);
			dwa_reset_cur_abord_tsk(wdev, coreid, work_job);
		}
	}
	spin_unlock_irqrestore(&wdev->job_lock, flags);
}

static void dwa_set_tsk_run_status(struct dwa_vdev *wdev, int top_id
	, struct dwa_job *job, struct dwa_task *tsk)
{
	atomic_set(&wdev->core[top_id].state, DWA_CORE_STATE_RUNNING);
	TRACE_DWA(DBG_INFO, "core(%d) state set(%d)\n", top_id, DWA_CORE_STATE_RUNNING);

	atomic_set(&tsk->state, DWA_TASK_STATE_RUNNING);

	dwa_proc_record_hw_tsk_start(job, tsk, top_id);
}

static void dwa_submit_hw(struct dwa_vdev *wdev, int top_id
	, struct dwa_job *job, struct dwa_task *tsk)
{
	struct dwa_cfg cfg;
	video_frame_s *in_frame, *out_frame;
	pixel_format_e pix_format;
	rotation_e rotation;
	unsigned long long mesh_addr;
	unsigned char num_of_plane, i;
	unsigned long flags;
	struct dwa_job *work_job;
	struct dwa_task *work_tsk;

	dwa_set_tsk_run_status(wdev, top_id, job, tsk);

	mesh_addr = tsk->attr.private_data[0];
	in_frame = &tsk->attr.img_in.video_frame;
	out_frame = &tsk->attr.img_out.video_frame;
	pix_format = in_frame->pixel_format;
	rotation = tsk->rotation;

	memset(&cfg, 0, sizeof(cfg));
	switch (out_frame->pixel_format) {
	case PIXEL_FORMAT_YUV_PLANAR_420:
		cfg.pix_fmt = YUV420p;
		num_of_plane = 3;
	break;
	case PIXEL_FORMAT_YUV_400:
		cfg.pix_fmt = YUV400;
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
	cfg.output_target = 1;//dram or sclr
	cfg.bgcolor = bgcolor;
	cfg.bdcolor = border_color;
	//cfg.bgcolor = tsk->bgcolor;
	//cfg.bdcolor = tsk->bdcolor;
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

	spin_lock_irqsave(&wdev->job_lock, flags);
	//tsk->coreid = top_id;
	job->coreid = top_id;

	list_add_tail(&job->node, &wdev->list.work_list[top_id]);
	work_job = list_first_entry_or_null(&wdev->list.work_list[top_id], struct dwa_job, node);

	list_add_tail(&tsk->node, &wdev->core[top_id].list.work_list);
	work_tsk = list_first_entry_or_null(&wdev->core[top_id].list.work_list, struct dwa_task, node);
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	if (unlikely(!work_job || !work_tsk)) {
		TRACE_DWA(DBG_WARN, "core_id:[%d] null work job tsk\n", top_id);
		return;
	}
	dwa_reset(top_id);
	dwa_init(top_id);
	dwa_intr_ctrl(DWA_INTR_EN_ALL, top_id);//0x7 if you want get mesh tbl id err status
	dwa_engine(&cfg, top_id);

	TRACE_DWA(DBG_DEBUG, "job[%px]\n", job);

	TRACE_DWA(DBG_DEBUG, "core_id:%d\n", top_id);
	TRACE_DWA(DBG_DEBUG, "update size src(%d %d) dst(%d %d)\n",
		cfg.src_width, cfg.src_height, cfg.dst_width, cfg.dst_height);
	TRACE_DWA(DBG_DEBUG, "update src-buf: %#llx-%#llx-%#llx\n",
		in_frame->phyaddr[0], in_frame->phyaddr[1], in_frame->phyaddr[2]);
	TRACE_DWA(DBG_DEBUG, "update dst-buf: %#llx-%#llx-%#llx\n",
		out_frame->phyaddr[0], out_frame->phyaddr[1], out_frame->phyaddr[2]);
	TRACE_DWA(DBG_DEBUG, "update mesh_id_addr(%#llx)\n", cfg.mesh_id);
	TRACE_DWA(DBG_DEBUG, "update output_target(%d)\n", cfg.output_target);
	TRACE_DWA(DBG_DEBUG, "update bgcolor(%#x), pix_fmt(%d)\n", cfg.bgcolor, cfg.pix_fmt);
	TRACE_DWA(DBG_DEBUG, "update src pitch(%d %d %d)\n"
		, cfg.src_buf[0].pitch, cfg.src_buf[1].pitch, cfg.src_buf[2].pitch);
	TRACE_DWA(DBG_DEBUG, "update dst pitch(%d %d %d)\n"
		, cfg.dst_buf[0].pitch, cfg.dst_buf[1].pitch, cfg.dst_buf[2].pitch);
#if 0
	dwa_dump_register(top_id);
#endif
}

static void dwa_submit_hw_cmdq(struct dwa_vdev *wdev, int top_id
	, struct dwa_job *job, struct dwa_task *last_tsk, struct dwa_task **tskq)
{
	struct dwa_cfg *cfg_q[DWA_JOB_MAX_TSK_NUM] = {NULL};
	union cmdq_set *cmdq_addr = NULL;
	video_frame_s *in_frame, *out_frame;
	pixel_format_e pix_format;
	rotation_e rotation;
	unsigned long long mesh_addr;
	unsigned char num_of_plane, i, tsk_idx;
	int tsk_num, cmdq_wq_ret;
	struct dwa_job *work_job;
	struct dwa_task *work_tsk;

	if (unlikely(!wdev || !job || !last_tsk || !tskq))
		return;

	tsk_num = atomic_read(&job->task_num);
	if (unlikely(last_tsk != tskq[tsk_num - 1])) {
		TRACE_DWA(DBG_ERR, "invalid last_tsk, not match with tskq\n");
		return;
	}

	cmdq_addr = kzalloc(sizeof(*cmdq_addr) * tsk_num * DWA_CMDQ_MAX_REG_CNT, GFP_ATOMIC);

	for (tsk_idx = 0; tsk_idx < tsk_num; tsk_idx++) {
		unsigned long flags;

		dwa_set_tsk_run_status(wdev, top_id, job, tskq[tsk_idx]);

		cfg_q[tsk_idx] = kzalloc(sizeof(struct dwa_cfg), GFP_ATOMIC);

		mesh_addr = tskq[tsk_idx]->attr.private_data[0];
		in_frame = &tskq[tsk_idx]->attr.img_in.video_frame;
		out_frame = &tskq[tsk_idx]->attr.img_out.video_frame;
		pix_format = in_frame->pixel_format;
		rotation = tskq[tsk_idx]->rotation;

		switch (out_frame->pixel_format) {
		case PIXEL_FORMAT_YUV_PLANAR_420:
			cfg_q[tsk_idx]->pix_fmt = YUV420p;
			num_of_plane = 3;
		break;
		case PIXEL_FORMAT_YUV_400:
			cfg_q[tsk_idx]->pix_fmt = YUV400;
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
		cfg_q[tsk_idx]->bgcolor = bgcolor;
		cfg_q[tsk_idx]->bdcolor = border_color;
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

			addr = out_frame->phyaddr[i];;

			cfg_q[tsk_idx]->dst_buf[i].addrl    = addr;
			cfg_q[tsk_idx]->dst_buf[i].addrh    = addr >> 32;
			cfg_q[tsk_idx]->dst_buf[i].pitch    = out_frame->stride[i];
			cfg_q[tsk_idx]->dst_buf[i].offset_x = cfg_q[tsk_idx]->src_buf[i].offset_y = 0;
		}

		spin_lock_irqsave(&wdev->job_lock, flags);
		//tskq[tsk_idx]->coreid = top_id;
		job->coreid = top_id;

		list_add_tail(&job->node, &wdev->list.work_list[top_id]);
		work_job = list_first_entry_or_null(&wdev->list.work_list[top_id], struct dwa_job, node);

		list_add_tail(&tskq[tsk_idx]->node, &wdev->core[top_id].list.work_list);
		work_tsk = list_first_entry_or_null(&wdev->core[top_id].list.work_list, struct dwa_task, node);

		wdev->core[top_id].cmdq_evt = false;
		spin_unlock_irqrestore(&wdev->job_lock, flags);

#if !USE_REAL_CMDQ //fake cmdq
		dwa_reset(top_id);
		dwa_init(top_id);
		dwa_intr_ctrl(DWA_INTR_EN_ALL, top_id);//0x0 cmdq need disable dwa intr
		dwa_engine(cfg_q[tsk_idx], top_id);

		if (tsk_idx != (tsk_num - 1)) {
			cmdq_wq_ret = wait_event_interruptible_timeout(wdev->core[top_id].cmdq_wq
				, wdev->core[top_id].cmdq_evt, DWA_EOF_WAIT_TIMEOUT_MS);
			if (cmdq_wq_ret <= 0) {
				TRACE_DWA(DBG_WARN, "dwa cdmq wait timeout, ret(%d)\n", cmdq_wq_ret);
				tsk_idx++;
				goto FREE_CMDQ_RES;
			}
			TRACE_DWA(DBG_DEBUG, "dwa cdmq wait done, ret(%d)\n", cmdq_wq_ret);
		}
#endif
	}

#if USE_REAL_CMDQ
	(void)cmdq_wq_ret;
	dwa_reset(top_id);
	dwa_init(top_id);
	dwa_intr_ctrl(DWA_INTR_EN_ALL, top_id);//0x0 cmdq need disable dwa intr
	dwa_engine_cmdq(top_id, (void *)cmdq_addr, cfg_q, tsk_num);
	goto FREE_CMDQ_RES;
#endif

FREE_CMDQ_RES:
	//last_tsk->coreid = top_id;

	TRACE_DWA(DBG_DEBUG, "cmd_start(%px)\n",cmdq_addr);
	for (i = 0; i < tsk_idx; i++) {
		TRACE_DWA(DBG_DEBUG, "tsk_id[%d]-core_id[%d]\n", i,  top_id);
		TRACE_DWA(DBG_DEBUG, "update size src(%d %d) dst(%d %d)\n",
			cfg_q[i]->src_width, cfg_q[i]->src_height
			, cfg_q[i]->dst_width, cfg_q[i]->dst_height);
		TRACE_DWA(DBG_DEBUG, "update src-buf: %#llx-%#llx-%#llx\n"
			, tskq[i]->attr.img_in.video_frame.phyaddr[0]
			, tskq[i]->attr.img_in.video_frame.phyaddr[1]
			, tskq[i]->attr.img_in.video_frame.phyaddr[2]);
		TRACE_DWA(DBG_DEBUG, "update dst-buf: %#llx-%#llx-%#llx\n"
			, tskq[i]->attr.img_out.video_frame.phyaddr[0]
			, tskq[i]->attr.img_out.video_frame.phyaddr[1]
			, tskq[i]->attr.img_out.video_frame.phyaddr[2]);
		TRACE_DWA(DBG_DEBUG, "update mesh_id_addr(%#llx)\n", cfg_q[i]->mesh_id);
		TRACE_DWA(DBG_DEBUG, "update output_target(%d)\n", cfg_q[i]->output_target);
		TRACE_DWA(DBG_DEBUG, "update bgcolor(%#x), pix_fmt(%d)\n"
			, cfg_q[i]->bgcolor, cfg_q[i]->pix_fmt);
		TRACE_DWA(DBG_DEBUG, "update src pitch(%d %d %d)\n"
			, cfg_q[i]->src_buf[0].pitch, cfg_q[i]->src_buf[1].pitch, cfg_q[i]->src_buf[2].pitch);
		TRACE_DWA(DBG_DEBUG, "update dst pitch(%d %d %d)\n"
			, cfg_q[i]->dst_buf[0].pitch, cfg_q[i]->dst_buf[1].pitch, cfg_q[i]->dst_buf[2].pitch);

		if (cfg_q[i])
			kfree(cfg_q[i]);
	}

if (cmdq_addr)
	kfree(cmdq_addr);

#if 0
	dwa_dump_register(top_id);
	//dwa_dump_cmdq(top_id);
#endif
}

static int dwa_try_submit_hw(struct dwa_vdev *wdev, struct dwa_job *job
	, struct dwa_task *tsk, unsigned char use_cmdq, struct dwa_task **tskq)
{
	int i, top_id, ret = -1;
	enum dwa_core_state state;
	bool finish;

	for (i = 0; i < wdev->core_num; i++) {
		finish = dwa_is_finish(i);
		state = atomic_read(&wdev->core[i].state);

		TRACE_DWA(DBG_INFO, "core[%d]-state[%d]-isfinish[%d]\n", i, state, finish);

		if ((state == DWA_CORE_STATE_IDLE) && finish) {
			top_id = i;
			//dwa_enable_dev_clk(top_id, true);
			if (use_cmdq)
				dwa_submit_hw_cmdq(wdev, top_id, job, tsk, tskq);
			else
				dwa_submit_hw(wdev, top_id, job, tsk);
			ret = 0;
			break;
		}
	}

	if (ret)
		TRACE_DWA(DBG_NOTICE, "dwa_submit_hw fail,hw busy\n");

	return ret;
}

static int dwa_try_submit_hw_cmdq(struct dwa_vdev *wdev, struct dwa_job *job
	, struct dwa_task **tskq, struct dwa_task *last_tsk)
{
	return dwa_try_submit_hw(wdev, job, last_tsk, true, tskq);
}

static unsigned char dwa_is_tskq_ready(struct dwa_task *tsk)
{
	if (unlikely(!tsk)) {
		TRACE_DWA(DBG_ERR, "cur tsk is null\n");
		return false;
	}

	if (unlikely(tsk->tsk_id < 0 || tsk->tsk_id >= DWA_JOB_MAX_TSK_NUM)) {
		TRACE_DWA(DBG_ERR, "cur tsk id(%d) invalid\n", tsk->tsk_id);
		return false;
	}

	if (unlikely(down_trylock(&tsk->sem))) {
		TRACE_DWA(DBG_ERR, "cur tsk(%px) get sem fail\n", tsk);
		return false;
	}
	return true;
}

static unsigned char dwa_get_idle_coreid(struct dwa_vdev *wdev)
{
	unsigned char coreid;
	enum dwa_core_state state;

	for (coreid = 0; coreid < wdev->core_num; coreid++) {
		state = atomic_read(&wdev->core[coreid].state);
		if (state == DWA_CORE_STATE_IDLE)
			break;
	}
	return coreid;
}

static bool dwa_have_idle_core(struct dwa_vdev *wdev)
{
	unsigned char coreid;
	enum dwa_core_state state;

	for (coreid = 0; coreid < wdev->core_num; coreid++) {
		state = atomic_read(&wdev->core[coreid].state);
		if (state == DWA_CORE_STATE_IDLE)
			return true;
	}
	return false;
}

static void dwa_try_commit_job(struct dwa_vdev *wdev, struct dwa_job *job)
{
	unsigned long flags;
	unsigned char use_cmdq = false, is_ready = true;
	unsigned char i = 0, tsk_num;
	struct dwa_task *tsk = NULL, *tmp_tsk = NULL, *tskq[DWA_JOB_MAX_TSK_NUM], *last_tsk = NULL;

	if (!wdev || !job)
		return;

	if (atomic_read(&job->job_state) != DWA_JOB_WORKING)
		return;

	if (!dwa_have_idle_core(wdev))
		return;

	tsk_num = atomic_read(&job->task_num);
	if (unlikely(tsk_num <= 0)) {
		TRACE_DWA(DBG_ERR, "cur job(%px) tsk_num(%d) invalid\n"
			, job, tsk_num);
		return;
	}

	if (tsk_num == 1) {
		use_cmdq = false;
		spin_lock_irqsave(&wdev->job_lock, flags);
		tsk = list_first_entry_or_null(&job->task_list, struct dwa_task, node);
		spin_unlock_irqrestore(&wdev->job_lock, flags);
		is_ready = dwa_is_tskq_ready(tsk);
	} else {
		use_cmdq = true;
		i = 0;
		spin_lock_irqsave(&wdev->job_lock, flags);
		list_for_each_entry_safe(tskq[i], tmp_tsk, &job->task_list, node) {
			is_ready &= dwa_is_tskq_ready(tskq[i]);
			last_tsk = tskq[i];
			i++;
		}
		spin_unlock_irqrestore(&wdev->job_lock, flags);
	}

	if (!is_ready) {
		TRACE_DWA(DBG_ERR, "cur job(%px) cur tsk(%px) is not ready\n", job, tsk);
		return;
	}

	job->use_cmdq = use_cmdq;
	if (use_cmdq) {
		if (unlikely(!last_tsk)) {
			TRACE_DWA(DBG_ERR, "invalid last_tsk,is null\n");
			return;
		}
		if (unlikely(!list_is_last(&last_tsk->node, &job->task_list))) {
			TRACE_DWA(DBG_ERR, "invalid last_tsk,is not last node\n");
			return;
		}
#if USE_REAL_CMDQ
		last_tsk->fn_tsk_cb = dwa_wkup_frm_done_work;
#else
		for (i = 0; i < atomic_read(&job->task_num); i++)
			tskq[i]->fn_tsk_cb = dwa_wkup_frm_done_work;
#endif
		dwa_try_submit_hw_cmdq(wdev, job, tskq, last_tsk);
	} else {
		tsk->fn_tsk_cb = dwa_wkup_frm_done_work;
		dwa_try_submit_hw(wdev, job, tsk, false, NULL);
	}
}

static int dwa_event_handler_th(void *data)
{
	struct dwa_vdev *wdev = (struct dwa_vdev *)data;
	unsigned long flags;
	struct dwa_job *job, *job_tmp;
	int ret;
	unsigned long idle_timeout = msecs_to_jiffies(DWA_IDLE_WAIT_TIMEOUT_MS);
	unsigned long eof_timeout = msecs_to_jiffies(DWA_EOF_WAIT_TIMEOUT_MS);
	unsigned long timeout = idle_timeout;
	unsigned char idle_coreid;
	bool have_idle_job;

	if (!wdev) {
		TRACE_DWA(DBG_ERR, "dwa vdev isn't created yet.\n");
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
		if (is_dwa_suspended())
			continue;

		/* timeout */
		if (!ret) {
			if (atomic_read(&wdev->state) == DWA_DEV_STATE_STOP) {
				timeout = idle_timeout;
				continue;
			} else if((atomic_read(&wdev->state) == DWA_DEV_STATE_RUNNING)
				&& list_empty(&wdev->job_list)) {
				timeout = idle_timeout;
				atomic_set(&wdev->state, DWA_DEV_STATE_STOP);
				up(&wdev->sem);
				continue;
			} else {
				dwa_try_reset_abort_job(wdev);
				continue;
			}
		}

		idle_coreid = dwa_get_idle_coreid(wdev);
		if (idle_coreid >= wdev->core_num) {
			TRACE_DWA(DBG_INFO, "dwa vdev hw busy, no idle core\n");
			goto continue_th;
		}

		if (list_empty(&wdev->job_list)) {
			TRACE_DWA(DBG_INFO, "job list empty\n");
			atomic_set(&wdev->state, DWA_DEV_STATE_STOP);
			up(&wdev->sem);
			goto continue_th;
		}

		have_idle_job = false;
		spin_lock_irqsave(&wdev->job_lock, flags);
		list_for_each_entry_safe(job, job_tmp, &wdev->job_list, node) {
			if (job->coreid == DWA_INVALID_CORE_ID) {
				TRACE_DWA(DBG_DEBUG, "got idle job[%px]\n", job);
				have_idle_job = true;
				break;
			}
		}

		if (!have_idle_job) { //have idle core but cur job is doing
			TRACE_DWA(DBG_INFO, "no idle job, job[%px] job_tmp[%px] coreid[%d]\n"
				, job, job_tmp, job_tmp->coreid);
			spin_unlock_irqrestore(&wdev->job_lock, flags);
			goto continue_th;
		}

		atomic_set(&job->job_state, DWA_JOB_WORKING);
		list_del(&job->node);
		spin_unlock_irqrestore(&wdev->job_lock, flags);

		dwa_proc_record_job_start(job);
		atomic_set(&wdev->state, DWA_DEV_STATE_RUNNING);
		dwa_try_commit_job(wdev, job);
continue_th:
		dwa_clr_evt_kth(wdev);

		/* Adjust timeout */
		timeout = list_empty(&wdev->job_list) ? idle_timeout : eof_timeout;
	}

	return 0;
}

static void dwa_cancel_cur_tsk(struct dwa_job *job)
{
	struct dwa_task *tsk = NULL;
	unsigned char i;

	if (unlikely(!job))
		return;
	if (unlikely(list_empty(&job->task_list))) {
			TRACE_DWA(DBG_NOTICE, "null tsk list\n");
			return;
	}

	for (i = 0; i < atomic_read(&job->task_num); i++) {
		tsk = list_first_entry_or_null(&job->task_list, struct dwa_task, node);
		if (tsk) {
			TRACE_DWA(DBG_DEBUG, "free tsk[%px]\n", tsk);
			list_del(&tsk->node);
			kfree(tsk);
			tsk = NULL;
		}
	}
}

/**************************************************************************
 *   Public APIs.
 **************************************************************************/
int dwa_begin_job(struct dwa_vdev *wdev, struct dwa_handle_data *data)
{
	struct dwa_job *job;
	unsigned long flags;
	int ret = 0;

	ret = dwa_check_null_ptr(wdev) ||
		dwa_check_null_ptr(data);
	if (ret)
		return ret;

	if (is_dwa_suspended()) {
		TRACE_DWA(DBG_ERR, "dwa dev suspend\n");
		return ERR_DWA_NOT_PERMITTED;
	}

	job = kzalloc(sizeof(struct dwa_job), GFP_ATOMIC);
	if (job == NULL) {
		TRACE_DWA(DBG_ERR, "malloc failed.\n");
		return ERR_DWA_NOBUF;
	}

	spin_lock_irqsave(&wdev->job_lock, flags);
	INIT_LIST_HEAD(&job->task_list);
	atomic_set(&job->job_state, DWA_JOB_CREAT);
	atomic_set(&job->task_num, 0);
	job->identity.sync_io = true;
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	data->handle = (u64)(uintptr_t)job;

	TRACE_DWA(DBG_DEBUG, "job[%px]++\n", job);

	return ret;
}

int dwa_end_job(struct dwa_vdev *wdev, unsigned long long handle)
{
	int ret = 0, sync_io_ret = 1;
	struct dwa_job *job = (struct dwa_job *)(uintptr_t)handle;
	unsigned long flags;
	int tsk_num = 0;
	unsigned long timeout = msecs_to_jiffies(DWA_SYNC_IO_WAIT_TIMEOUT_MS);
	struct dwa_task *tsk, *tmp_tsk;

	ret = dwa_check_null_ptr(wdev) ||
		dwa_check_null_ptr(job);
	if (ret)
		return ret;

	if (list_empty(&job->task_list)) {
		TRACE_DWA(DBG_DEBUG, "no task in job.\n");
		return ERR_DWA_NOT_PERMITTED;
	}
	TRACE_DWA(DBG_INFO, "job[%px]\n", job);

	spin_lock_irqsave(&wdev->job_lock, flags);
	atomic_set(&job->job_state, DWA_JOB_WAIT);
	list_add_tail(&job->node, &wdev->job_list);
	wdev->job_cnt++;

	list_for_each_entry_safe(tsk, tmp_tsk, &job->task_list, node) {
		up(&tsk->sem);
		tsk_num++;
		tsk->tsk_id = tsk_num - 1;
	}
	atomic_set(&job->task_num, tsk_num);
	job->coreid = DWA_INVALID_CORE_ID;
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	dwa_proc_commit_job(job);

	dwa_notify_wkup_evt_kth(wdev, DWA_EVENT_WKUP);

	if (job->identity.sync_io) {
		init_waitqueue_head(&job->job_done_wq);
		job->job_done_evt = false;
		sync_io_ret = wait_event_timeout(job->job_done_wq, job->job_done_evt, timeout);
		if (sync_io_ret <= 0) {
			TRACE_DWA(DBG_WARN, "end job[%px] fail,timeout, ret(%d)\n", job, sync_io_ret);
			ret = -1;
			spin_lock_irqsave(&wdev->job_lock, flags);
			wdev->job_cnt--;
			spin_unlock_irqrestore(&wdev->job_lock, flags);
		} else
			ret = 0;
	}

	TRACE_DWA(DBG_INFO, "jobname[%s] sync_io=%d, ret=%d\n", job->identity.name, job->identity.sync_io, ret);

	if (job->identity.sync_io) {
		kfree(job);
		job = NULL;
	}

	return ret;
}

int dwa_cancel_job(struct dwa_vdev *wdev, unsigned long long handle)
{
	int ret = 0;
	struct dwa_job *job = (struct dwa_job *)(uintptr_t)handle;
	struct dwa_job *job_tmp, *work_job, *wait_job;
	unsigned long flags;
	unsigned char coreid;
	bool needfreeJob = false;

	ret = dwa_check_null_ptr(wdev) ||
		dwa_check_null_ptr(job);
	if (ret)
		return ret;

	spin_lock_irqsave(&wdev->job_lock, flags);
	if (atomic_read(&job->job_state) == DWA_JOB_CREAT) {
		needfreeJob = true;
		goto FREE_JOB;
	}

	list_for_each_entry_safe(wait_job, job_tmp, &wdev->job_list, node) {
		if (job == wait_job) {
			TRACE_DWA(DBG_DEBUG, "cancel wait job(%px)\n", wait_job);
			atomic_set(&wdev->state, DWA_DEV_STATE_RUNNING);
			needfreeJob = true;
			dwa_cancel_cur_tsk(job);
			list_del(&job->node);
			break;
		}
	}

	for (coreid = 0; coreid < DWA_DEV_MAX_CNT; coreid++) {
		list_for_each_entry_safe(work_job, job_tmp, &wdev->list.work_list[coreid], node) {
			if (job == work_job) {
				TRACE_DWA(DBG_DEBUG, "cancel work job(%px)\n", wait_job);
				atomic_set(&wdev->state, DWA_DEV_STATE_RUNNING);
				atomic_set(&wdev->core[coreid].state, DWA_CORE_STATE_IDLE);
				dwa_core_deinit(coreid);
				needfreeJob = true;
				dwa_cancel_cur_tsk(job);
				list_del(&job->node);
				break;
			}
		}
	}

FREE_JOB:
	if (needfreeJob) {
		TRACE_DWA(DBG_DEBUG, "free job[%px]\n", job);
		// kfree(job);
	}
	spin_unlock_irqrestore(&wdev->job_lock, flags);
	TRACE_DWA(DBG_DEBUG, "++\n");

	return ret;
}

int dwa_get_work_job(struct dwa_vdev *wdev, struct dwa_handle_data *data)
{
	int ret = 0;
	struct dwa_job *job = NULL;
	unsigned long flags;
	unsigned char coreid;

	ret = dwa_check_null_ptr(wdev) ||
		dwa_check_null_ptr(data);
	if (ret)
		return ret;

	spin_lock_irqsave(&wdev->job_lock, flags);
	for (coreid = 0; coreid < wdev->core_num; coreid++) {
		job = list_first_entry_or_null(&wdev->list.work_list[coreid], struct dwa_job, node);
		if (job) {
			break;
		}
	}
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	data->handle = (u64)(uintptr_t)job;

	TRACE_DWA(DBG_DEBUG, "job[%px]\n", job);

	return ret;
}

int dwa_set_identity(struct dwa_vdev *wdev,
			  struct dwa_identity_attr *identity)
{
	struct dwa_job *job;
	unsigned long flags;
	unsigned long long handle;

	if( dwa_check_null_ptr(wdev) ||
		dwa_check_null_ptr(identity)) {
		TRACE_DWA(DBG_ERR, "null dev or identity_attr\n");
		return -1;
	}
	handle = identity->handle;

	spin_lock_irqsave(&wdev->job_lock, flags);
	job = (struct dwa_job *)(uintptr_t)handle;
	if (!job) {
		TRACE_DWA(DBG_ERR, "null job handle\n");
		return -1;
	}

	memcpy(&job->identity, &identity->attr, sizeof(identity->attr));
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	TRACE_DWA(DBG_DEBUG, "sync_io:%d, name:%s, id:%d\n"
		, job->identity.sync_io, job->identity.name, job->identity.id);
	return 0;
}

int dwa_add_rotation_task(struct dwa_vdev *wdev,
			  struct dwa_task_attr *attr)
{
	struct dwa_job *job;
	struct dwa_task *tsk;
	unsigned long long handle;
	unsigned long flags;
	int ret = 0;

	if (!dwa_check_param_is_valid(wdev, attr))
		return -1;
	handle = attr->handle;

	ret = dwa_rot_check_size(attr->rotation, attr);
	if (ret)
		return ret;

	spin_lock_irqsave(&wdev->job_lock, flags);
	job = (struct dwa_job *)(uintptr_t)handle;
	tsk = kzalloc(sizeof(*tsk), GFP_ATOMIC);
	sema_init(&tsk->sem, 0);

	memcpy(&tsk->attr, attr, sizeof(tsk->attr));
	tsk->type = DWA_TASK_TYPE_ROT;
	tsk->rotation = attr->rotation;
	atomic_set(&tsk->state, DWA_TASK_STATE_WAIT);
	atomic_add(1, &job->task_num);
	list_add_tail(&tsk->node, &job->task_list);
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	TRACE_DWA(DBG_DEBUG, "job[%px] tsk(%px)\n", job, tsk);

	return ret;
}

int dwa_add_ldc_task(struct dwa_vdev *wdev, struct dwa_task_attr *attr)
{
	struct dwa_job *job;
	struct dwa_task *tsk;
	unsigned long long handle;
	unsigned long flags;
	int ret = 0;

	if (!dwa_check_param_is_valid(wdev, attr))
		return -1;
	handle = attr->handle;

#if 0
	ret = dwa_rot_check_size(attr->rotation, attr);
	if (ret)
		return ret;
#endif

	spin_lock_irqsave(&wdev->job_lock, flags);
	job = (struct dwa_job *)(uintptr_t)handle;
	tsk = kzalloc(sizeof(*tsk), GFP_ATOMIC);
	sema_init(&tsk->sem, 0);

	memcpy(&tsk->attr, attr, sizeof(tsk->attr));
	tsk->type = DWA_TASK_TYPE_LDC;
	tsk->rotation = attr->rotation;
	atomic_set(&tsk->state, DWA_TASK_STATE_WAIT);
	atomic_add(1, &job->task_num);
	list_add_tail(&tsk->node, &job->task_list);
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	TRACE_DWA(DBG_DEBUG, "job[%px] tsk(%px)\n", job, tsk);

	return ret;
}

int dwa_add_cor_task(struct dwa_vdev *wdev, struct dwa_task_attr *attr)
{
	struct dwa_job *job;
	struct dwa_task *tsk;
	unsigned long long handle;
	unsigned long flags;
	int ret = 0;

	if (!dwa_check_param_is_valid(wdev, attr))
		return -1;

	handle = attr->handle;

	ret = dwa_cor_check_param(attr);
	if (ret)
		return ret;

	spin_lock_irqsave(&wdev->job_lock, flags);
	job = (struct dwa_job *)(uintptr_t)handle;
	tsk = kzalloc(sizeof(*tsk), GFP_ATOMIC);
	sema_init(&tsk->sem, 0);

	memcpy(&tsk->attr, attr, sizeof(tsk->attr));
	tsk->type = DWA_TASK_TYPE_FISHEYE;
	tsk->rotation = attr->rotation;
	atomic_set(&tsk->state, DWA_TASK_STATE_WAIT);
	atomic_add(1, &job->task_num);
	list_add_tail(&tsk->node, &job->task_list);
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	TRACE_DWA(DBG_DEBUG, "job[%px] tsk(%px)\n", job, tsk);

	return ret;
}

int dwa_add_warp_task(struct dwa_vdev *wdev, struct dwa_task_attr *attr)
{
	struct dwa_job *job;
	struct dwa_task *tsk;
	unsigned long long handle;
	unsigned long flags;
	int ret = 0;

	if (!dwa_check_param_is_valid(wdev, attr))
		return -1;

	handle = attr->handle;

	spin_lock_irqsave(&wdev->job_lock, flags);
	job = (struct dwa_job *)(uintptr_t)handle;
	tsk = kzalloc(sizeof(*tsk), GFP_ATOMIC);
	sema_init(&tsk->sem, 0);

	memcpy(&tsk->attr, attr, sizeof(tsk->attr));
	tsk->type = DWA_TASK_TYPE_WARP;
	tsk->rotation = attr->rotation;
	atomic_set(&tsk->state, DWA_TASK_STATE_WAIT);
	atomic_add(1, &job->task_num);
	list_add_tail(&tsk->node, &job->task_list);
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	TRACE_DWA(DBG_DEBUG, "job[%px] tsk(%px)\n", job, tsk);

	return ret;
}

int dwa_add_affine_task(struct dwa_vdev *wdev, struct dwa_task_attr *attr)
{
	struct dwa_job *job;
	struct dwa_task *tsk;
	unsigned long long handle;
	unsigned long flags;
	int ret = 0;

	if (!dwa_check_param_is_valid(wdev, attr))
		return -1;
	handle = attr->handle;

	ret = dwa_affine_check_param(attr);
	if (ret)
		return ret;

	spin_lock_irqsave(&wdev->job_lock, flags);
	job = (struct dwa_job *)(uintptr_t)handle;
	tsk = kzalloc(sizeof(*tsk), GFP_ATOMIC);
	sema_init(&tsk->sem, 0);

	memcpy(&tsk->attr, attr, sizeof(tsk->attr));
	tsk->type = DWA_TASK_TYPE_AFFINE;
	tsk->rotation = attr->rotation;
	atomic_set(&tsk->state, DWA_TASK_STATE_WAIT);
	atomic_add(1, &job->task_num);
	list_add_tail(&tsk->node, &job->task_list);
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	TRACE_DWA(DBG_DEBUG, "job[%px] tsk(%px)\n", job, tsk);

	return ret;
}

int dwa_get_chn_frame(struct dwa_vdev *wdev, struct dwa_identity_attr *identity
	, video_frame_info_s *pstvideo_frame, int s32milli_sec)
{
	int ret;
	unsigned long flags;
	struct dwa_vb_done *vb_done = NULL, *vb_done_tmp = NULL;
	bool ismatch = false;

	TRACE_DWA(DBG_DEBUG, "++\n");

	ret = dwa_check_null_ptr(pstvideo_frame)
		|| dwa_check_null_ptr(wdev)
		|| dwa_check_null_ptr(identity);
	if (ret != 0)
		return ret;

	memset(pstvideo_frame, 0, sizeof(*pstvideo_frame));
	if (s32milli_sec <= 0) {
		if (down_trylock(&wdev->vb_doneq.sem)) {
			TRACE_DWA(DBG_ERR, "cannot get sem, doneq not ready\n");
			return ERR_DWA_SYS_NOTREADY;
		}
	} else {
		TRACE_DWA(DBG_DEBUG, "sem.count[%d]\n", wdev->vb_doneq.sem.count);
		ret = down_timeout(&wdev->vb_doneq.sem, msecs_to_jiffies(s32milli_sec));
		if (ret == -ETIME) {
			TRACE_DWA(DBG_ERR, "get sem timeout, doneq not ready\n");
			return ret;
		}
		TRACE_DWA(DBG_DEBUG, "sem.count[%d]\n", wdev->vb_doneq.sem.count);
	}

	spin_lock_irqsave(&wdev->job_lock, flags);
	if (list_empty(&wdev->vb_doneq.doneq)) {
		TRACE_DWA(DBG_ERR, "vb_doneq is empty\n");
		spin_unlock_irqrestore(&wdev->job_lock, flags);
		return ERR_DWA_NOBUF;
	}

	list_for_each_entry_safe(vb_done, vb_done_tmp, &wdev->vb_doneq.doneq, node) {
		if (!vb_done) {
			TRACE_DWA(DBG_ERR, "vb_done is null\n");
			spin_unlock_irqrestore(&wdev->job_lock, flags);
			return ERR_DWA_NOBUF;
		}

		if (dwa_identity_is_match(&vb_done->job.identity, &identity->attr)) {
			TRACE_DWA(DBG_DEBUG, "vb_doneq identity[%d-%d-%s] is match [%d-%d-%s]\n"
				, vb_done->job.identity.mod_id, vb_done->job.identity.id, vb_done->job.identity.name
				, identity->attr.mod_id, identity->attr.id, identity->attr.name);
			ismatch = true;
			break;
		}
	}

	if (!ismatch) {
		TRACE_DWA(DBG_DEBUG, "vb_doneq[%px] identity[%d-%d-%s] not match [%d-%d-%s]\n"
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

	TRACE_DWA(DBG_DEBUG, "end to get pstvideo_frame width:%d height:%d buf:0x%llx\n"
		, pstvideo_frame->video_frame.width
		, pstvideo_frame->video_frame.height
		, pstvideo_frame->video_frame.phyaddr[0]);
	TRACE_DWA(DBG_DEBUG, "--\n");

	return ret;
}

/**************************************************************************
 *   internal APIs.
 **************************************************************************/
int dwa_sw_init(struct dwa_vdev *wdev)
{
	struct sched_param tsk;
	int ret = 0;
	unsigned char coreid;

	ret = dwa_check_null_ptr(wdev);
	if (ret)
		return ret;

	INIT_LIST_HEAD(&wdev->job_list);
	INIT_LIST_HEAD(&wdev->vb_doneq.doneq);
	init_waitqueue_head(&wdev->wait);
	sema_init(&wdev->vb_doneq.sem, 0);
	sema_init(&wdev->sem, 0);

	spin_lock_init(&wdev->job_lock);
	wdev->core_num = DWA_DEV_MAX_CNT;
	wdev->evt = DWA_EVENT_BUSY_OR_NOT_STAT;
	wdev->vb_pool = VB_INVALID_POOLID;

	for (coreid = 0; coreid < wdev->core_num; coreid++) {
#if DWA_USE_WORKQUEUE
		INIT_WORK(&wdev->core[coreid].work_frm_done, dwa_work_frm_done);
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

	wdev->thread = kthread_run(dwa_event_handler_th, (void *)wdev, "dwa_event_handler_th");
	if (IS_ERR(wdev->thread)) {
		TRACE_DWA(DBG_ERR, "failed to create dwa kthread\n");
		return -1;
	}

	// Same as sched_set_fifo in linux 5.x
	tsk.sched_priority = MAX_USER_RT_PRIO - 10;
	ret = sched_setscheduler(wdev->thread, SCHED_FIFO, &tsk);
	if (ret)
		TRACE_DWA(DBG_WARN, "dwa thread priority update failed: %d\n", ret);

	/*wdev->workqueue = create_singlethread_workqueue("dwa_workqueue");
	if (!wdev->workqueue) {
		TRACE_DWA(DBG_ERR, "dwa dev create_workqueue failed.\n");
		return -2;
	}

	INIT_WORK(&wdev->work_frm_done, dwa_work_frm_done);*/

	return ret;
}

void dwa_sw_deinit(struct dwa_vdev *wdev)
{
	unsigned char coreid;

	if (dwa_check_null_ptr(wdev) != 0)
		return;

	/*if (wdev->workqueue)
		destroy_workqueue(wdev->workqueue);*/

#if DWA_USE_WORKQUEUE
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
			TRACE_DWA(DBG_ERR, "fail to stop dwa kthread\n");
	}

	list_del_init(&wdev->job_list);
	list_del_init(&wdev->vb_doneq.doneq);
}

int dwa_suspend_handler(void)
{
	int ret;
	struct dwa_vdev * dev = dwa_get_dev();
	struct dwa_job *job;

	if (unlikely(!dev)) {
		TRACE_DWA(DBG_ERR, "dwa_dev is null\n");
		return ERR_DWA_NULL_PTR;
	}

	if (!dev->thread) {
		TRACE_DWA(DBG_ERR, "dwa thread not initialized yet\n");
		return ERR_DWA_SYS_NOTREADY;
	}

	if (dev->job_cnt > 0 || !list_empty(&dev->job_list) || atomic_read(&dev->state) == DWA_DEV_STATE_RUNNING) {
		sema_init(&dev->sem, 0);
		ret = down_timeout(&dev->sem, msecs_to_jiffies(DWA_IDLE_WAIT_TIMEOUT_MS));
		if (ret == -ETIME) {
			TRACE_DWA(DBG_ERR, "get sem timeout, dev not idle yet\n");
			return ret;
		}
	}
	atomic_set(&dev->state, DWA_DEV_STATE_STOP);

	spin_lock(&dev->job_lock);
	while(!list_empty(&dev->job_list)) {
		job = list_first_entry_or_null(&dev->job_list, struct dwa_job, node);
		list_del(&job->node);
	}
	dev->job_cnt = 0;
	spin_unlock(&dev->job_lock);

	TRACE_DWA(DBG_WARN, "suspend handler+\n");

	return 0;
}

int dwa_resume_handler(void)
{
	struct dwa_vdev * dev = dwa_get_dev();

	if (unlikely(!dev)) {
		TRACE_DWA(DBG_ERR, "dwa_dev is null\n");
		return ERR_DWA_NULL_PTR;
	}

	if (!dev->thread) {
		TRACE_DWA(DBG_ERR, "dwa thread not initialized yet\n");
		return ERR_DWA_SYS_NOTREADY;
	}

	atomic_set(&dev->state, DWA_DEV_STATE_RUNNING);

	TRACE_DWA(DBG_WARN, "resume handler+\n");

	return 0;
}

