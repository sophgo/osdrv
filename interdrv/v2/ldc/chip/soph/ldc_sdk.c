
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <uapi/linux/sched/types.h>

#include <linux/cvi_comm_video.h>
#include <linux/cvi_comm_gdc.h>
#include <linux/ldc_uapi.h>

#include <base_cb.h>
#include <vb.h>
#include <sys.h>
#include "ldc_debug.h"
#include "cvi_vip_ldc.h"
#include "ldc_sdk.h"
#include "ldc_common.h"
#include "ldc.h"
#include "cmdq.h"
#include "ion.h"
#include "vbq.h"
#include "cvi_vip_ldc_proc.h"
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

static enum ENUM_MODULES_ID convert_cb_id(MOD_ID_E enModId)
{
	if (enModId == CVI_ID_VI)
		return E_MODULE_VI;
	else if (enModId == CVI_ID_VO)
		return E_MODULE_VO;
	else if (enModId == CVI_ID_VPSS)
		return E_MODULE_VPSS;

	return E_MODULE_BUTT;
}

static MOD_ID_E convert_mod_id(enum ENUM_MODULES_ID cbModId)
{
	if (cbModId == E_MODULE_VI)
		return CVI_ID_VI;
	else if (cbModId == E_MODULE_VO)
		return CVI_ID_VO;
	else if (cbModId == E_MODULE_VPSS)
		return CVI_ID_VPSS;

	return CVI_ID_BUTT;
}

int ldc_exec_cb(void *dev, enum ENUM_MODULES_ID caller, u32 cmd, void *arg)
{
	struct cvi_ldc_vdev *wdev = (struct cvi_ldc_vdev *)dev;
	struct mesh_gdc_cfg *cfg;
	MOD_ID_E enModId;
	int rc = -1;

	switch (cmd) {
		case LDC_CB_MESH_GDC_OP: {
			mutex_lock(&g_mesh_lock);
			cfg = (struct mesh_gdc_cfg *)arg;
			enModId = convert_mod_id(caller);
			mutex_unlock(&g_mesh_lock);

			if (enModId == CVI_ID_BUTT) {
				CVI_TRACE_LDC(CVI_DBG_WARN, "invalid mod do GDC_OP\n");
				return CVI_FAILURE;
			}
			rc = mesh_gdc_do_op(wdev, cfg->usage, cfg->pUsageParam
				, cfg->vb_in, cfg->enPixFormat, cfg->mesh_addr
				,cfg->sync_io, cfg->pcbParam, cfg->cbParamSize
				, enModId, cfg->enRotation);
			break;
		}
		default: {
			CVI_TRACE_LDC(CVI_DBG_WARN, "invalid cb CMD\n");
			break;
		}
	}

	return rc;
}

int ldc_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_LDC);
}

int ldc_reg_cb(struct cvi_ldc_vdev *wdev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_LDC;
	reg_cb.dev		= (void *)wdev;
	reg_cb.cb		= ldc_exec_cb;

	return base_reg_module_cb(&reg_cb);
}

static void ldc_op_done_cb(MOD_ID_E enModId, void *pParam, VB_BLK blk)
{
	struct ldc_op_done_cfg cfg;
	struct base_exe_m_cb exe_cb;
	enum ENUM_MODULES_ID callee = convert_cb_id(enModId);

	CVI_TRACE_LDC(CVI_DBG_DEBUG, "callee=%d, enModId(%d)\n", callee, enModId);
	cfg.pParam = pParam;
	cfg.blk = blk;

	exe_cb.callee = callee;
	exe_cb.caller = E_MODULE_LDC;
	exe_cb.cmd_id = LDC_CB_GDC_OP_DONE;
	exe_cb.data   = &cfg;
	base_exe_module_cb(&exe_cb);
}

static void ldc_hdl_hw_tsk_cb(struct cvi_ldc_vdev *wdev, struct ldc_job *job
	, struct ldc_task *tsk, bool is_last)
{
	bool is_internal;
	VB_BLK blk_in = VB_INVALID_HANDLE, blk_out = VB_INVALID_HANDLE;
	MOD_ID_E enModId;
	u8 isLastTask;
	struct ldc_vb_done *vb_done = NULL;
	unsigned long flags;

	if (unlikely(!wdev || !job || !tsk)) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "invalid param\n");
		return;
	}
	is_internal = (tsk->attr.reserved == CVI_GDC_MAGIC);
	enModId = job->identity.enModId;

	CVI_TRACE_LDC(CVI_DBG_DEBUG, "is_internal=%d\n", is_internal);

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
		isLastTask = (u8)tsk->attr.au64privateData[1];

		CVI_TRACE_LDC(CVI_DBG_DEBUG, "isLastTask=%d, blk_in(pa=0x%llx), blk_out(pa=0x%llx)\n",
				isLastTask,
				(unsigned long long)tsk->attr.stImgIn.stVFrame.u64PhyAddr[0],
				(unsigned long long)tsk->attr.stImgOut.stVFrame.u64PhyAddr[0]);

		blk_in = vb_phys_addr2handle(tsk->attr.stImgIn.stVFrame.u64PhyAddr[0]);
		blk_out = vb_phys_addr2handle(tsk->attr.stImgOut.stVFrame.u64PhyAddr[0]);
		if (blk_out == VB_INVALID_HANDLE)
			CVI_TRACE_LDC(CVI_DBG_ERR, "blk_out is invalid vb_blk, no callback to(%d)\n", enModId);
		else {
			atomic_long_fetch_and(~BIT(CVI_ID_GDC), &((struct vb_s *)blk_out)->mod_ids);
			if (isLastTask)
				ldc_op_done_cb(enModId, (void *)(uintptr_t)tsk->attr.au64privateData[2], blk_out);
		}

		// User space:
		//   Caller always assign callback.
		//   Null callback used for internal ldc sub job.
		// Kernel space:
		//  !isLastTask used for internal ldc sub job.
		if (isLastTask && blk_in != VB_INVALID_HANDLE) {
			atomic_long_fetch_and(~BIT(CVI_ID_GDC), &((struct vb_s *)blk_in)->mod_ids);
			vb_release_block(blk_in);
		} else if (!isLastTask) {
			vfree((void *)(uintptr_t)tsk->attr.au64privateData[2]);
		}
	} else {
		if (!job->identity.syncIo && is_last) {
			vb_done = kzalloc(sizeof(*vb_done), GFP_ATOMIC);

			spin_lock_irqsave(&wdev->job_lock, flags);
			memcpy(&vb_done->stImgOut, &tsk->attr.stImgOut, sizeof(vb_done->stImgOut));
			memcpy(&vb_done->job, job, sizeof(*job));
			list_add_tail(&vb_done->node, &wdev->vb_doneq.doneq);
			spin_unlock_irqrestore(&wdev->job_lock, flags);

			CVI_TRACE_LDC(CVI_DBG_DEBUG, "sem.count[%d]\n", wdev->vb_doneq.sem.count);
			up(&wdev->vb_doneq.sem);
			CVI_TRACE_LDC(CVI_DBG_DEBUG, "sem.count[%d]\n", wdev->vb_doneq.sem.count);

			CVI_TRACE_LDC(CVI_DBG_DEBUG, "vb_done->stImgOut[%llx-%d]\n"
				, vb_done->stImgOut.stVFrame.u64PhyAddr[0], vb_done->stImgOut.stVFrame.u32Width);
			CVI_TRACE_LDC(CVI_DBG_DEBUG, "vb_doneq identity[%d-%d-%s]\n"
				, vb_done->job.identity.enModId, vb_done->job.identity.u32ID, vb_done->job.identity.Name);
		}
	}
}

static void ldc_wkup_cmdq_tsk(struct ldc_core *core) {
#if USE_REAL_CMDQ
	(void)(core);
#else
	CVI_TRACE_LDC(CVI_DBG_DEBUG, "ldc_wkup_cmdq_tsk\n");

	core->cmdq_evt = true;
	wake_up_interruptible(&core->cmdq_wq);
#endif
}

static void ldc_notify_wkup_evt_kth(void *data, enum ldc_wait_evt evt)
{
	struct cvi_ldc_vdev *dev = (struct cvi_ldc_vdev *)data;
	unsigned long flags;

	if(!dev) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "ldc vdev isn't created yet.\n");
		return;
	}

	spin_lock_irqsave(&dev->job_lock, flags);
	dev->evt |= evt;
	CVI_TRACE_LDC(CVI_DBG_DEBUG, "evt[%d], dev evt[%d]\n", evt, dev->evt);
	spin_unlock_irqrestore(&dev->job_lock, flags);

	wake_up_interruptible(&dev->wait);
}

static void ldc_clr_evt_kth(void *data)
{
	struct cvi_ldc_vdev *dev = (struct cvi_ldc_vdev *)data;
	unsigned long flags;
	enum ldc_wait_evt evt;

	if(!dev) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "ldc vdev isn't created yet.\n");
		return;
	}

	spin_lock_irqsave(&dev->job_lock, flags);
	evt = dev->evt;
	dev->evt &= ~evt;
	CVI_TRACE_LDC(CVI_DBG_DEBUG, "evt[%d], dev evt[%d]\n", evt, dev->evt);
	spin_unlock_irqrestore(&dev->job_lock, flags);
}

static void ldc_work_handle_job_done(struct cvi_ldc_vdev *dev, struct ldc_job *job, u8 irq_coreid)
{
	unsigned long flags;
	struct fasync_struct *fasync;

	if (!job) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "null job\n");
		return;
	}

	atomic_set(&job->enJobState, LDC_JOB_END);

	ldc_proc_record_job_done(job);

	spin_lock_irqsave(&dev->job_lock, flags);
	dev->job_cnt--;
	spin_unlock_irqrestore(&dev->job_lock, flags);
	ldc_notify_wkup_evt_kth(dev, LDC_EVENT_EOF);

	if (job->identity.syncIo) {
		CVI_TRACE_LDC(CVI_DBG_INFO, "job[%px]\n", job);
		job->job_done_evt = true;
		wake_up_interruptible(&job->job_done_wq);
	} else {
		kfree(job);
		job = NULL;
		fasync = ldc_get_dev_fasync();
		kill_fasync(&fasync, SIGIO, POLL_IN);
	}
	CVI_TRACE_LDC(CVI_DBG_DEBUG, "job done\n");
}

static void ldc_work_handle_tsk_done(struct cvi_ldc_vdev *dev
	, struct ldc_task *tsk, struct ldc_job *job, u8 irq_coreid, bool is_last_tsk)
{
	unsigned long flags;

	if (!tsk) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "null tsk\n");
		return;
	}

	if (atomic_read(&tsk->state) == LDC_TASK_STATE_RUNNING) {
		atomic_set(&tsk->state, LDC_TASK_STATE_DONE);
		atomic_dec(&job->task_num);

		ldc_hdl_hw_tsk_cb(dev, job, tsk, is_last_tsk);

		ldc_proc_record_hw_tsk_done(job, tsk);

		spin_lock_irqsave(&dev->job_lock, flags);
		CVI_TRACE_LDC(CVI_DBG_DEBUG, "tsk[%px]\n", tsk);
		kfree(tsk);
		tsk = NULL;
		spin_unlock_irqrestore(&dev->job_lock, flags);

		CVI_TRACE_LDC(CVI_DBG_DEBUG, "tsk done, is last tsk[%d]\n", is_last_tsk);
		if (is_last_tsk) {
			ldc_work_handle_job_done(dev, job, irq_coreid);
		}
	} else {
		CVI_TRACE_LDC(CVI_DBG_ERR, "invalid tsk state(%d).\n"
			, (enum ldc_task_state)atomic_read(&tsk->state));
	}
}

void ldc_work_handle_frm_done(struct cvi_ldc_vdev *dev, struct ldc_core *core)
{
	struct ldc_job *job;
	struct ldc_task *tsk;
	unsigned long flags;
	u8 irq_coreid;
	bool is_last_tsk = false;

	if(!dev) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "ldc vdev isn't created yet.\n");
		return;
	}

	atomic_set(&core->state, LDC_CORE_STATE_IDLE);
	irq_coreid = (u8)core->dev_type;
	CVI_TRACE_LDC(CVI_DBG_INFO, "core(%d) state set(%d)\n", irq_coreid, LDC_CORE_STATE_IDLE);

	if (unlikely(irq_coreid) >= CVI_DEV_LDC_MAX)
		return;

	spin_lock_irqsave(&dev->job_lock, flags);
	job = list_first_entry_or_null(&dev->list.done_list[irq_coreid], struct ldc_job, node);
	spin_unlock_irqrestore(&dev->job_lock, flags);

	if (unlikely(!job))
		return;

	if (atomic_read(&job->enJobState) == LDC_JOB_WORKING) {
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
		CVI_TRACE_LDC(CVI_DBG_ERR, "invalid job(%px) state(%d).\n"
			, job, (enum ldc_job_state)atomic_read(&job->enJobState));
	}
}

#if LDC_USE_WORKQUEUE
static void ldc_work_frm_done(struct work_struct *work)//intr post handle
{
	struct ldc_core *core = container_of(work, struct ldc_core, work_frm_done);
	struct cvi_ldc_vdev *dev = ldc_get_dev();

	if(!core || !dev) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "ldc vdev or core isn't created yet.\n");
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
		CVI_TRACE_LDC(CVI_DBG_ERR, "ldc core isn't created yet.\n");
		return;
	}

#if LDC_USE_WORKQUEUE
	//queue_work(dev->workqueue, &dev->work_frm_done);//wakeup post handle
	schedule_work(&core->work_frm_done);
#else
	ldc_work_handle_frm_done(ldc_get_dev(), core);
#endif
}

static void ldc_reset_cur_abord_job(struct cvi_ldc_vdev *dev, u8 coreid, struct ldc_job *job)
{
#if USE_EXCEPTION_HDL
	struct ldc_task *tsk, tsk_tmp;

	if (unlikely(!dev)) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "ldc vdev isn't created yet.\n");
		return;
	}

	if (unlikely(!job)) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "err job is null\n");
		return;
	}

	if (list_empty(&job->task_list)) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "err tsklist is null\n");
		return;
	}

	list_for_each_entry_safe(tsk, tsk_tmp, &job->task_list, node) {
		if (unlikely(!tsk)) {
			CVI_TRACE_LDC(CVI_DBG_ERR, "cur job(%px) cur tsk is null\n", job);
			continue;
		}

		if (unlikely(tsk->tsk_id < 0 || tsk->tsk_id >= LDC_JOB_MAX_TSK_NUM)) {
			CVI_TRACE_LDC(CVI_DBG_ERR, "cur job(%px) cur tsk id(%d) invalid\n", job, tsk->tsk_id);
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

static void ldc_try_reset_abort_job(struct cvi_ldc_vdev *wdev)
{
	u8 coreid;
	unsigned long flags;
	struct ldc_job *work_job;

	spin_lock_irqsave(&wdev->job_lock, flags);
	for (coreid = 0; coreid < wdev->core_num; coreid++) {
		work_job = list_first_entry_or_null(&wdev->list.work_list[coreid], struct ldc_job, node);
		if (unlikely(!work_job)) {
			CVI_TRACE_LDC(CVI_DBG_NOTICE, "ldc vdev core[%d] select timeout,hw busy\n", coreid);
			ldc_reset_cur_abord_job(wdev, coreid, work_job);
		}
	}
	spin_unlock_irqrestore(&wdev->job_lock, flags);
}

static void ldc_set_tsk_run_status(struct cvi_ldc_vdev *wdev, int top_id
	, struct ldc_job *job, struct ldc_task *tsk)
{
	atomic_set(&wdev->core[top_id].state, LDC_CORE_STATE_RUNNING);
	CVI_TRACE_LDC(CVI_DBG_INFO, "core(%d) state set(%d)\n", top_id, LDC_CORE_STATE_RUNNING);

	atomic_set(&tsk->state, LDC_TASK_STATE_RUNNING);

	ldc_proc_record_hw_tsk_start(job, tsk, top_id);
}

static void ldc_submit_hw(struct cvi_ldc_vdev *wdev, int top_id
	, struct ldc_job *job, struct ldc_task *tsk)
{
	struct ldc_cfg cfg;
	VIDEO_FRAME_S *in_frame, *out_frame;
	PIXEL_FORMAT_E enPixFormat;
	ROTATION_E enRotation;
	u64 mesh_addr;
	u8 num_of_plane, extend_haddr;
	unsigned long flags;
	struct ldc_job *work_job;
	struct ldc_task *work_tsk;

	ldc_set_tsk_run_status(wdev, top_id, job, tsk);

	mesh_addr = tsk->attr.au64privateData[0];
	mesh_addr = (mesh_addr != DEFAULT_MESH_PADDR) ? mesh_addr : 0;
	in_frame = &tsk->attr.stImgIn.stVFrame;
	out_frame = &tsk->attr.stImgOut.stVFrame;
	enPixFormat = in_frame->enPixelFormat;
	enRotation = tsk->enRotation;

	memset(&cfg, 0, sizeof(cfg));
	switch (out_frame->enPixelFormat) {
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

	switch (enRotation) {
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
	cfg.src_width = ALIGN(in_frame->u32Width, 64);
	cfg.src_height = ALIGN(in_frame->u32Height, 64);
	cfg.ras_width = cfg.src_width;
	cfg.ras_height = cfg.src_height;

	if (cfg.map_base == 0)
		cfg.map_bypass = true;
	else
		cfg.map_bypass = false;

	cfg.src_xstart = 0;
	cfg.src_xend = in_frame->u32Width - 1;

	extend_haddr = in_frame->u64PhyAddr[0] >> 33;
	cfg.extend_haddr = ((extend_haddr << 28) | (extend_haddr << 13) | extend_haddr);

	cfg.src_y_base = in_frame->u64PhyAddr[0];
	cfg.dst_y_base = out_frame->u64PhyAddr[0];
	if (num_of_plane == 2) {
		cfg.src_c_base = in_frame->u64PhyAddr[1];
		cfg.dst_c_base = out_frame->u64PhyAddr[1];
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
		CVI_TRACE_LDC(CVI_DBG_WARN, "core_id:[%d] null work job tsk\n", top_id);
		return;
	}

	ldc_reset(top_id);
	ldc_init(top_id);
	ldc_intr_ctrl(LDC_INTR_EN_ALL, top_id);
	ldc_engine(&cfg, top_id);

	CVI_TRACE_LDC(CVI_DBG_INFO, "job[%px]\n", job);
	CVI_TRACE_LDC(CVI_DBG_DEBUG, "core_id:%d\n", top_id);
	CVI_TRACE_LDC(CVI_DBG_DEBUG, "update size src(%d %d)\n", cfg.src_width, cfg.src_height);
	CVI_TRACE_LDC(CVI_DBG_DEBUG, "update src-buf: %#llx-%#llx-%#llx\n",
		in_frame->u64PhyAddr[0], in_frame->u64PhyAddr[1], in_frame->u64PhyAddr[2]);
	CVI_TRACE_LDC(CVI_DBG_DEBUG, "update dst-buf: %#llx-%#llx-%#llx\n",
		out_frame->u64PhyAddr[0], out_frame->u64PhyAddr[1], out_frame->u64PhyAddr[2]);
	CVI_TRACE_LDC(CVI_DBG_DEBUG, "update mesh_id_addr(%#llx)\n", cfg.map_base);
	CVI_TRACE_LDC(CVI_DBG_DEBUG, "update bgcolor(%#x), pix_fmt(%d)\n", cfg.bgcolor, cfg.pix_fmt);
#if 0
	ldc_dump_register(top_id);
#endif
}

static void ldc_submit_hw_cmdq(struct cvi_ldc_vdev *wdev, int top_id
	, struct ldc_job *job, struct ldc_task *last_tsk, struct ldc_task **tskq)
{
	struct ldc_cfg *cfg_q[LDC_JOB_MAX_TSK_NUM] = {NULL};
	union cmdq_set *cmdq_addr = NULL;
	VIDEO_FRAME_S *in_frame, *out_frame;
	PIXEL_FORMAT_E enPixFormat;
	ROTATION_E enRotation;
	u64 mesh_addr;
	u8 num_of_plane, i, tsk_idx, extend_haddr;
	int tsk_num, cmdq_wq_ret;
	struct ldc_job *work_job;
	struct ldc_task *work_tsk;

	if (unlikely(!wdev || !job || !last_tsk || !tskq))
		return;

	tsk_num = atomic_read(&job->task_num);
	if (unlikely(last_tsk != tskq[tsk_num - 1])) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "invalid last_tsk, not match with tskq\n");
		return;
	}

	cmdq_addr = kzalloc(sizeof(*cmdq_addr) * tsk_num * LDC_CMDQ_MAX_REG_CNT, GFP_ATOMIC);

	for (tsk_idx = 0; tsk_idx < tsk_num; tsk_idx++) {
		unsigned long flags;

		ldc_set_tsk_run_status(wdev, top_id, job, tskq[tsk_idx]);

		cfg_q[tsk_idx] = kzalloc(sizeof(struct ldc_cfg), GFP_ATOMIC);

		mesh_addr = tskq[tsk_idx]->attr.au64privateData[0];
		mesh_addr = (mesh_addr != DEFAULT_MESH_PADDR) ? mesh_addr : 0;
		in_frame = &tskq[tsk_idx]->attr.stImgIn.stVFrame;
		out_frame = &tskq[tsk_idx]->attr.stImgOut.stVFrame;
		enPixFormat = in_frame->enPixelFormat;
		enRotation = tskq[tsk_idx]->enRotation;

		switch (out_frame->enPixelFormat) {
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

		switch (enRotation) {
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
		cfg_q[tsk_idx]->src_width = ALIGN(in_frame->u32Width, LDC_SIZE_ALIGN);
		cfg_q[tsk_idx]->src_height = ALIGN(in_frame->u32Height, LDC_SIZE_ALIGN);
		cfg_q[tsk_idx]->ras_width = cfg_q[tsk_idx]->src_width;
		cfg_q[tsk_idx]->ras_height = cfg_q[tsk_idx]->src_height;

		if (cfg_q[tsk_idx]->map_base == 0)
			cfg_q[tsk_idx]->map_bypass = true;
		else
			cfg_q[tsk_idx]->map_bypass = false;

		cfg_q[tsk_idx]->src_xstart = 0;
		cfg_q[tsk_idx]->src_xend = in_frame->u32Width - 1;

		extend_haddr = in_frame->u64PhyAddr[0] >> 33;
		cfg_q[tsk_idx]->extend_haddr = ((extend_haddr << 28) | (extend_haddr << 13) | extend_haddr);

		cfg_q[tsk_idx]->src_y_base = in_frame->u64PhyAddr[0];
		cfg_q[tsk_idx]->dst_y_base = out_frame->u64PhyAddr[0];
		if (num_of_plane == 2) {
			cfg_q[tsk_idx]->src_c_base = in_frame->u64PhyAddr[1];
			cfg_q[tsk_idx]->dst_c_base = out_frame->u64PhyAddr[1];
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
				CVI_TRACE_LDC(CVI_DBG_WARN, "ldc cdmq wait timeout, ret(%d)\n", cmdq_wq_ret);
				tsk_idx++;
				goto FREE_CMDQ_RES;
			}
			CVI_TRACE_LDC(CVI_DBG_DEBUG, "ldc cdmq wait done, ret(%d)\n", cmdq_wq_ret);
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

	CVI_TRACE_LDC(CVI_DBG_DEBUG, "cmd_start(%px)\n",cmdq_addr);
	for (i = 0; i < tsk_idx; i++) {
		CVI_TRACE_LDC(CVI_DBG_DEBUG, "tsk_id[%d]-core_id[%d]\n", i,  top_id);
		CVI_TRACE_LDC(CVI_DBG_DEBUG, "update size src(%d %d)\n", cfg_q[i]->src_width, cfg_q[i]->src_height);
		CVI_TRACE_LDC(CVI_DBG_DEBUG, "update src-buf: %#llx-%#llx\n",
			cfg_q[i]->src_y_base, cfg_q[i]->src_c_base);
		CVI_TRACE_LDC(CVI_DBG_DEBUG, "update dst-buf: %#llx-%#llx\n",
			cfg_q[i]->dst_y_base, cfg_q[i]->dst_c_base);
		CVI_TRACE_LDC(CVI_DBG_DEBUG, "update mesh_id_addr(%#llx)\n", cfg_q[i]->map_base);
		CVI_TRACE_LDC(CVI_DBG_DEBUG, "update bgcolor(%#x), pix_fmt(%d)\n"
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

static int ldc_try_submit_hw(struct cvi_ldc_vdev *wdev, struct ldc_job *job
	, struct ldc_task *tsk, u8 use_cmdq, struct ldc_task **tskq)
{
	int i, top_id, ret = -1;
	enum ldc_core_state state;
	bool finish;

	for (i = 0; i < wdev->core_num; i++) {
		finish = ldc_is_finish(i);
		state = atomic_read(&wdev->core[i].state);

		CVI_TRACE_LDC(CVI_DBG_INFO, "core[%d]-state[%d]-isfinish[%d]\n", i, state, finish);

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
		CVI_TRACE_LDC(CVI_DBG_NOTICE, "ldc_submit_hw fail,hw busy\n");

	return ret;
}

static int ldc_try_submit_hw_cmdq(struct cvi_ldc_vdev *wdev, struct ldc_job *job
	, struct ldc_task **tskq, struct ldc_task *last_tsk)
{
	return ldc_try_submit_hw(wdev, job, last_tsk, true, tskq);
}

static u8 ldc_is_tsk_ready(struct ldc_task *tsk)
{
	if (unlikely(!tsk)) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "cur tsk is null\n");
		return CVI_FALSE;
	}

	if (unlikely(tsk->tsk_id < 0 || tsk->tsk_id >= LDC_JOB_MAX_TSK_NUM)) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "cur tsk id(%d) invalid\n", tsk->tsk_id);
		return CVI_FALSE;
	}

	if (unlikely(down_trylock(&tsk->sem))) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "cur tsk(%px) get sem fail\n", tsk);
		return CVI_FALSE;
	}
	return CVI_TRUE;
}

static u8 ldc_get_idle_coreid(struct cvi_ldc_vdev *wdev)
{
	u8 coreid;
	enum ldc_core_state state;

	for (coreid = 0; coreid < wdev->core_num; coreid++) {
		state = atomic_read(&wdev->core[coreid].state);
		if (state == LDC_CORE_STATE_IDLE)
			break;
	}
	return coreid;
}

static bool ldc_have_idle_core(struct cvi_ldc_vdev *wdev)
{
	u8 coreid;
	enum ldc_core_state state;

	for (coreid = 0; coreid < wdev->core_num; coreid++) {
		state = atomic_read(&wdev->core[coreid].state);
		if (state == LDC_CORE_STATE_IDLE)
			return true;
	}
	return false;
}

static void ldc_try_commit_job(struct cvi_ldc_vdev *wdev, struct ldc_job *job)
{
	unsigned long flags;
	u8 use_cmdq = false, is_ready = CVI_TRUE;
	u8 i = 0, tsk_num;
	struct ldc_task *tsk = NULL, *tmp_tsk = NULL, *tskq[LDC_JOB_MAX_TSK_NUM], *last_tsk = NULL;

	if (!wdev || !job)
		return;

	if (atomic_read(&job->enJobState) != LDC_JOB_WORKING)
		return;

	if (!ldc_have_idle_core(wdev))
		return;

	tsk_num = atomic_read(&job->task_num);
	if (unlikely(tsk_num <= 0)) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "cur job(%px) tsk_num(%d) invalid\n"
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
		CVI_TRACE_LDC(CVI_DBG_ERR, "cur job(%px) cur tsk(%px) is not ready\n", job, tsk);
		return;
	}

	job->use_cmdq = use_cmdq;
	if (use_cmdq) {
		if (unlikely(!last_tsk)) {
			CVI_TRACE_LDC(CVI_DBG_ERR, "invalid last_tsk,is null\n");
			return;
		}
		if (unlikely(!list_is_last(&last_tsk->node, &job->task_list))) {
			CVI_TRACE_LDC(CVI_DBG_ERR, "invalid last_tsk,is not last node\n");
			return;
		}
#if USE_REAL_CMDQ
		last_tsk->pfnTskCB = ldc_wkup_frm_done_work;
#else
		for (i = 0; i < atomic_read(&job->task_num); i++)
			tskq[i]->pfnTskCB = ldc_wkup_frm_done_work;
#endif
		ldc_try_submit_hw_cmdq(wdev, job, tskq, last_tsk);
	} else {
		tsk->pfnTskCB = ldc_wkup_frm_done_work;
		ldc_try_submit_hw(wdev, job, tsk, false, NULL);
	}
}

static int ldc_event_handler_th(void *data)
{
	struct cvi_ldc_vdev *wdev = (struct cvi_ldc_vdev *)data;
	unsigned long flags;
	struct ldc_job *job, *job_tmp;
	int ret;
	unsigned long idle_timeout = msecs_to_jiffies(LDC_IDLE_WAIT_TIMEOUT_MS);
	unsigned long eof_timeout = msecs_to_jiffies(LDC_EOF_WAIT_TIMEOUT_MS);
	unsigned long timeout = idle_timeout;
	u8 idle_coreid;
	bool have_idle_job;

	if (!wdev) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "ldc vdev isn't created yet.\n");
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
			CVI_TRACE_LDC(CVI_DBG_INFO, "ldc vdev hw busy, no idle core\n");
			goto continue_th;
		}

		if (list_empty(&wdev->job_list)) {
			CVI_TRACE_LDC(CVI_DBG_DEBUG, "job list empty\n");
			atomic_set(&wdev->state, LDC_DEV_STATE_STOP);
			up(&wdev->sem);
			goto continue_th;
		}

		have_idle_job = false;
		spin_lock_irqsave(&wdev->job_lock, flags);
		list_for_each_entry_safe(job, job_tmp, &wdev->job_list, node) {
			if (job->coreid == LDC_INVALID_CORE_ID) {
				CVI_TRACE_LDC(CVI_DBG_DEBUG, "got idle job[%px]\n", job);
				have_idle_job = true;
				break;
			}
		}

		if (!have_idle_job) { //have idle core but cur job is doing
			CVI_TRACE_LDC(CVI_DBG_INFO, "no idle job, job[%px] job_tmp[%px] coreid[%d]\n"
				, job, job_tmp, job_tmp->coreid);
			spin_unlock_irqrestore(&wdev->job_lock, flags);
			goto continue_th;
		}

		atomic_set(&job->enJobState, LDC_JOB_WORKING);
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
	u8 i;

	if (unlikely(!job))
		return;
	if (unlikely(list_empty(&job->task_list))) {
		CVI_TRACE_LDC(CVI_DBG_NOTICE, "null tsk list\n");
		return;
	}

	for (i = 0; i < atomic_read(&job->task_num); i++) {
		tsk = list_first_entry_or_null(&job->task_list, struct ldc_task, node);
		if (tsk) {
			CVI_TRACE_LDC(CVI_DBG_DEBUG, "free tsk[%px]\n", tsk);
			list_del(&tsk->node);
			kfree(tsk);
			tsk = NULL;
		}
	}
}

/**************************************************************************
 *   Public APIs.
 **************************************************************************/
s32 ldc_begin_job(struct cvi_ldc_vdev *wdev, struct gdc_handle_data *data)
{
	struct ldc_job *job;
	unsigned long flags;
	s32 ret = CVI_SUCCESS;

	ret = LDC_CHECK_NULL_PTR(wdev) ||
		LDC_CHECK_NULL_PTR(data);
	if (ret)
		return ret;

	if (is_ldc_suspended()) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "ldc dev suspend\n");
		return CVI_ERR_GDC_NOT_PERMITTED;
	}

	job = kzalloc(sizeof(struct ldc_job), GFP_ATOMIC);
	if (job == NULL) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "malloc failed.\n");
		return CVI_ERR_GDC_NOBUF;
	}

	spin_lock_irqsave(&wdev->job_lock, flags);
	INIT_LIST_HEAD(&job->task_list);
	atomic_set(&job->enJobState, LDC_JOB_CREAT);
	atomic_set(&job->task_num, 0);
	job->identity.syncIo = true;
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	data->handle = (u64)(uintptr_t)job;

	CVI_TRACE_LDC(CVI_DBG_DEBUG, "job[%px]++\n", job);

	return ret;
}

s32 ldc_end_job(struct cvi_ldc_vdev *wdev, u64 hHandle)
{
	s32 ret = CVI_SUCCESS, sync_io_ret = 1;
	struct ldc_job *job = (struct ldc_job *)(uintptr_t)hHandle;
	unsigned long flags;
	int tsk_num = 0;
	unsigned long timeout = msecs_to_jiffies(LDC_SYNC_IO_WAIT_TIMEOUT_MS);
	struct ldc_task *tsk, *tmp_tsk;

	ret = LDC_CHECK_NULL_PTR(wdev) ||
		LDC_CHECK_NULL_PTR(job);
	if (ret)
		return ret;

	if (list_empty(&job->task_list)) {
		CVI_TRACE_LDC(CVI_DBG_DEBUG, "no task in job.\n");
		return CVI_ERR_GDC_NOT_PERMITTED;
	}
	CVI_TRACE_LDC(CVI_DBG_INFO, "job[%px]\n", job);

	spin_lock_irqsave(&wdev->job_lock, flags);
	atomic_set(&job->enJobState, LDC_JOB_WAIT);
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

	if (job->identity.syncIo) {
		init_waitqueue_head(&job->job_done_wq);
		job->job_done_evt = false;
		sync_io_ret = wait_event_interruptible_timeout(job->job_done_wq, job->job_done_evt, timeout);
		if (sync_io_ret <= 0) {
			CVI_TRACE_LDC(CVI_DBG_WARN, "end job[%px] fail,timeout, ret(%d)\n", job, sync_io_ret);
			ret = -1;
		} else
			ret = 0;
	}

	CVI_TRACE_LDC(CVI_DBG_INFO, "jobname[%s] sync_io=%d, ret=%d\n", job->identity.Name, job->identity.syncIo, ret);

	if (job->identity.syncIo) {
		kfree(job);
		job = NULL;
	}

	return ret;
}

s32 ldc_cancel_job(struct cvi_ldc_vdev *wdev, u64 hHandle)
{
	s32 ret = CVI_SUCCESS;
	struct ldc_job *job = (struct ldc_job *)(uintptr_t)hHandle;
	struct ldc_job *job_tmp, *work_job, *wait_job;
	unsigned long flags;
	u8 coreid;
	bool needfreeJob = false;

	ret = LDC_CHECK_NULL_PTR(wdev) ||
		LDC_CHECK_NULL_PTR(job);
	if (ret)
		return ret;

	spin_lock_irqsave(&wdev->job_lock, flags);
	if (atomic_read(&job->enJobState) == LDC_JOB_CREAT) {
		needfreeJob = true;
		goto FREE_JOB;
	}

	list_for_each_entry_safe(wait_job, job_tmp, &wdev->job_list, node) {
		if (job == wait_job) {
			CVI_TRACE_LDC(CVI_DBG_DEBUG, "cancel wait job(%px)\n", wait_job);
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
				CVI_TRACE_LDC(CVI_DBG_DEBUG, "cancel work job(%px)\n", wait_job);
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
		CVI_TRACE_LDC(CVI_DBG_DEBUG, "free job[%px]\n", job);
		// kfree(job);
	}
	spin_unlock_irqrestore(&wdev->job_lock, flags);
	CVI_TRACE_LDC(CVI_DBG_DEBUG, "++\n");

	return ret;
}

s32 ldc_get_work_job(struct cvi_ldc_vdev *wdev, struct gdc_handle_data *data)
{
	s32 ret = CVI_SUCCESS;
	struct ldc_job *job = NULL;
	unsigned long flags;
	u8 coreid;

	ret = LDC_CHECK_NULL_PTR(wdev) ||
		LDC_CHECK_NULL_PTR(data);
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

	CVI_TRACE_LDC(CVI_DBG_DEBUG, "job[%px]\n", job);

	return ret;
}

s32 ldc_set_identity(struct cvi_ldc_vdev *wdev,
			  struct gdc_identity_attr *identity)
{
	struct ldc_job *job;
	unsigned long flags;
	u64 handle;

	if( LDC_CHECK_NULL_PTR(wdev) ||
		LDC_CHECK_NULL_PTR(identity)) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "null dev or identity_attr\n");
		return CVI_FAILURE;
	}
	handle = identity->handle;

	spin_lock_irqsave(&wdev->job_lock, flags);
	job = (struct ldc_job *)(uintptr_t)handle;
	if (!job) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "null job handle\n");
		return CVI_FAILURE;
	}

	memcpy(&job->identity, &identity->attr, sizeof(identity->attr));
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	CVI_TRACE_LDC(CVI_DBG_DEBUG, "syncIo:%d, name:%s, u32ID:%d\n"
		, job->identity.syncIo, job->identity.Name, job->identity.u32ID);
	return CVI_SUCCESS;
}

s32 ldc_add_rotation_task(struct cvi_ldc_vdev *wdev,
			  struct gdc_task_attr *attr)
{
	struct ldc_job *job;
	struct ldc_task *tsk;
	u64 handle;
	unsigned long flags;
	s32 ret = CVI_SUCCESS;

	if (!LDC_CHECK_PARAM_IS_VALID(wdev, attr))
		return CVI_FAILURE;
	handle = attr->handle;

	ret = LDC_ROT_CHECK_SIZE(attr->enRotation, attr);
	if (ret)
		return ret;

	spin_lock_irqsave(&wdev->job_lock, flags);
	job = (struct ldc_job *)(uintptr_t)handle;
	tsk = kzalloc(sizeof(*tsk), GFP_ATOMIC);
	sema_init(&tsk->sem, 0);

	memcpy(&tsk->attr, attr, sizeof(tsk->attr));
	tsk->type = LDC_TASK_TYPE_ROT;
	tsk->enRotation = attr->enRotation;
	atomic_set(&tsk->state, LDC_TASK_STATE_WAIT);
	atomic_add(1, &job->task_num);
	list_add_tail(&tsk->node, &job->task_list);
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	CVI_TRACE_LDC(CVI_DBG_DEBUG, "job[%px] tsk(%px)\n", job, tsk);

	return ret;
}

s32 ldc_add_ldc_task(struct cvi_ldc_vdev *wdev, struct gdc_task_attr *attr)
{
	struct ldc_job *job;
	struct ldc_task *tsk;
	u64 handle;
	unsigned long flags;
	s32 ret = CVI_SUCCESS;

	if (!LDC_CHECK_PARAM_IS_VALID(wdev, attr))
		return CVI_FAILURE;
	handle = attr->handle;

#if 0
	ret = LDC_ROT_CHECK_SIZE(attr->enRotation, attr);
	if (ret)
		return ret;
#endif

	spin_lock_irqsave(&wdev->job_lock, flags);
	job = (struct ldc_job *)(uintptr_t)handle;
	tsk = kzalloc(sizeof(*tsk), GFP_ATOMIC);
	sema_init(&tsk->sem, 0);

	memcpy(&tsk->attr, attr, sizeof(tsk->attr));
	tsk->type = LDC_TASK_TYPE_LDC;
	tsk->enRotation = attr->enRotation;
	atomic_set(&tsk->state, LDC_TASK_STATE_WAIT);
	atomic_add(1, &job->task_num);
	list_add_tail(&tsk->node, &job->task_list);
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	CVI_TRACE_LDC(CVI_DBG_DEBUG, "job[%px] tsk(%px)\n", job, tsk);

	return ret;
}

s32 ldc_get_chn_frame(struct cvi_ldc_vdev *wdev, struct gdc_identity_attr *identity
	, VIDEO_FRAME_INFO_S *pstVideoFrame, s32 s32MilliSec)
{
	s32 ret;
	unsigned long flags;
	struct ldc_vb_done *vb_done = NULL, *vb_done_tmp = NULL;
	bool ismatch = false;

	CVI_TRACE_LDC(CVI_DBG_DEBUG, "++\n");
	ret = LDC_CHECK_NULL_PTR(pstVideoFrame)
		|| LDC_CHECK_NULL_PTR(wdev)
		|| LDC_CHECK_NULL_PTR(identity);
	if (ret != CVI_SUCCESS)
		return ret;

	memset(pstVideoFrame, 0, sizeof(*pstVideoFrame));
	if (s32MilliSec <= 0) {
		if (down_trylock(&wdev->vb_doneq.sem)) {
			CVI_TRACE_LDC(CVI_DBG_ERR, "cannot get sem, doneq not ready\n");
			return CVI_ERR_GDC_SYS_NOTREADY;
		}
	} else {
		CVI_TRACE_LDC(CVI_DBG_DEBUG, "sem.count[%d]\n", wdev->vb_doneq.sem.count);
		ret = down_timeout(&wdev->vb_doneq.sem, msecs_to_jiffies(s32MilliSec));
		if (ret == -ETIME) {
			CVI_TRACE_LDC(CVI_DBG_ERR, "get sem timeout, doneq not ready\n");
			return ret;
		}
		CVI_TRACE_LDC(CVI_DBG_DEBUG, "sem.count[%d]\n", wdev->vb_doneq.sem.count);
	}

	spin_lock_irqsave(&wdev->job_lock, flags);
	if (list_empty(&wdev->vb_doneq.doneq)) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "vb_doneq is empty\n");
		spin_unlock_irqrestore(&wdev->job_lock, flags);
		return CVI_ERR_GDC_NOBUF;
	}

	list_for_each_entry_safe(vb_done, vb_done_tmp, &wdev->vb_doneq.doneq, node) {
		if (!vb_done) {
			CVI_TRACE_LDC(CVI_DBG_ERR, "vb_done is null\n");
			spin_unlock_irqrestore(&wdev->job_lock, flags);
			return CVI_ERR_GDC_NOBUF;
		}

		if (LDC_IDENTITY_IS_MATCH(&vb_done->job.identity, &identity->attr)) {
			CVI_TRACE_LDC(CVI_DBG_DEBUG, "vb_doneq identity[%d-%d-%s] is match [%d-%d-%s]\n"
				, vb_done->job.identity.enModId, vb_done->job.identity.u32ID, vb_done->job.identity.Name
				, identity->attr.enModId, identity->attr.u32ID, identity->attr.Name);
			ismatch = true;
			break;
		}
	}

	if (!ismatch) {
		CVI_TRACE_LDC(CVI_DBG_DEBUG, "vb_doneq[%px] identity[%d-%d-%s] not match [%d-%d-%s]\n"
			, vb_done_tmp, vb_done_tmp->job.identity.enModId, vb_done_tmp->job.identity.u32ID, vb_done_tmp->job.identity.Name
			, identity->attr.enModId, identity->attr.u32ID, identity->attr.Name);
		up(&wdev->vb_doneq.sem);
		spin_unlock_irqrestore(&wdev->job_lock, flags);
		return CVI_ERR_GDC_NOBUF;
	}

	list_del(&vb_done->node);

	memcpy(pstVideoFrame, &vb_done->stImgOut, sizeof(*pstVideoFrame));
	kfree(vb_done);
	vb_done = NULL;

	spin_unlock_irqrestore(&wdev->job_lock, flags);

	CVI_TRACE_LDC(CVI_DBG_DEBUG, "end to get pstVideoFrame width:%d height:%d buf:0x%llx\n"
		, pstVideoFrame->stVFrame.u32Width
		, pstVideoFrame->stVFrame.u32Height
		, pstVideoFrame->stVFrame.u64PhyAddr[0]);
	CVI_TRACE_LDC(CVI_DBG_DEBUG, "--\n");

	return ret;
}

s32 ldc_attach_vb_pool(VB_POOL VbPool)
{
	unsigned long flags;
	struct cvi_ldc_vdev *wdev = ldc_get_dev();

	if (!wdev)
		return CVI_FAILURE;

	spin_lock_irqsave(&wdev->job_lock, flags);
	wdev->VbPool = VbPool;
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	CVI_TRACE_LDC(CVI_DBG_DEBUG, "attach vb pool(%d)\n", VbPool);
	return CVI_SUCCESS;
}

s32 ldc_detach_vb_pool(void)
{
	unsigned long flags;
	struct cvi_ldc_vdev *wdev = ldc_get_dev();

	if (!wdev)
		return CVI_FAILURE;

	spin_lock_irqsave(&wdev->job_lock, flags);
	wdev->VbPool = VB_INVALID_POOLID;
	spin_unlock_irqrestore(&wdev->job_lock, flags);

	CVI_TRACE_LDC(CVI_DBG_DEBUG, "dettach vb pool\n");
	return CVI_SUCCESS;
}

/**************************************************************************
 *   internal APIs.
 **************************************************************************/
int cvi_ldc_sw_init(struct cvi_ldc_vdev *wdev)
{
	struct sched_param tsk;
	s32 ret = CVI_SUCCESS;
	u8 coreid;

	ret = LDC_CHECK_NULL_PTR(wdev);
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
	wdev->VbPool = VB_INVALID_POOLID;

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
		CVI_TRACE_LDC(CVI_DBG_ERR, "failed to create ldc kthread\n");
		return -1;
	}

	// Same as sched_set_fifo in linux 5.x
	tsk.sched_priority = MAX_USER_RT_PRIO - 10;
	ret = sched_setscheduler(wdev->thread, SCHED_FIFO, &tsk);
	if (ret)
		CVI_TRACE_LDC(CVI_DBG_WARN, "ldc thread priority update failed: %d\n", ret);

	/*wdev->workqueue = create_singlethread_workqueue("ldc_workqueue");
	if (!wdev->workqueue) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "ldc dev create_workqueue failed.\n");
		return -2;
	}

	INIT_WORK(&wdev->work_frm_done, ldc_work_frm_done);*/

	return ret;
}

void cvi_ldc_sw_deinit(struct cvi_ldc_vdev *wdev)
{
	u8 coreid;
	if (LDC_CHECK_NULL_PTR(wdev) != CVI_SUCCESS)
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
			CVI_TRACE_LDC(CVI_DBG_ERR, "fail to stop ldc kthread\n");
	}

	list_del_init(&wdev->job_list);
	list_del_init(&wdev->vb_doneq.doneq);
}

s32 ldc_suspend_handler(void)
{
	int ret;
	struct cvi_ldc_vdev * dev = ldc_get_dev();

	if (unlikely(!dev)) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "ldc_dev is null\n");
		return CVI_ERR_GDC_NULL_PTR;
	}

	if (!dev->thread) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "ldc thread not initialized yet\n");
		return CVI_ERR_GDC_SYS_NOTREADY;
	}

	if (dev->job_cnt > 0 || !list_empty(&dev->job_list) || atomic_read(&dev->state) == LDC_DEV_STATE_RUNNING) {
		sema_init(&dev->sem, 0);
		ret = down_timeout(&dev->sem, msecs_to_jiffies(LDC_IDLE_WAIT_TIMEOUT_MS));
		if (ret == -ETIME) {
			CVI_TRACE_LDC(CVI_DBG_ERR, "get sem timeout, dev not idle yet\n");
			return ret;
		}
	}
	atomic_set(&dev->state, LDC_DEV_STATE_STOP);

	CVI_TRACE_LDC(CVI_DBG_DEBUG, "suspend handler+\n");
	return CVI_SUCCESS;
}

s32 ldc_resume_handler(void)
{
	struct cvi_ldc_vdev * dev = ldc_get_dev();

	if (unlikely(!dev)) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "ldc_dev is null\n");
		return CVI_ERR_GDC_NULL_PTR;
	}

	if (!dev->thread) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "ldc thread not initialized yet\n");
		return CVI_ERR_GDC_SYS_NOTREADY;
	}

	atomic_set(&dev->state, LDC_DEV_STATE_RUNNING);

	CVI_TRACE_LDC(CVI_DBG_DEBUG, "resume handler+\n");
	return CVI_SUCCESS;
}

