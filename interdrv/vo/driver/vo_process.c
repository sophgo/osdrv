
#include "vo_debug.h"
#include "vo_process.h"
#include "vo_sdk_layer.h"
#include "comm_buffer.h"
#include "base_cb.h"
#include "ldc_cb.h"
#include "vo_cb.h"
#include "gfbg_cb.h"
#include "ion.h"

/*******************************************************
 *  MACRO defines
 ******************************************************/
#define WAIT_TIMEOUT_MS  200
/*******************************************************
 *  Global variables
 ******************************************************/
struct vo_ctx *g_vo_ctx;
static void *plogodata = NULL;
const char *const disp_irq_name[DISP_MAX_INST] = {"disp"};

int default_overlay_bind_priority[VO_MAX_DEV_NUM][VO_MAX_GRAPHIC_LAYER_IN_DEV] = {
	{VO_LAYER_G0},//dev0 bind G0
};

struct _vo_gdc_cb_param {
	mmf_chn_s chn;
	enum gdc_usage usage;
};

#define IS_VB_OFFSET_INVALID(buf) \
	((buf).offset_left < 0 || (buf).offset_right < 0 || \
	 (buf).offset_top < 0 || (buf).offset_bottom < 0 || \
	 ((unsigned int)((buf).offset_left + (buf).offset_right) > (buf).size.width) || \
	 ((unsigned int)((buf).offset_top + (buf).offset_bottom) > (buf).size.height))

#define FRC_INVALID(frame_rate_ctrl)	\
	(frame_rate_ctrl.dst_frame_rate <= 0 || frame_rate_ctrl.src_frame_rate <= 0 ||	\
	 frame_rate_ctrl.dst_frame_rate >= frame_rate_ctrl.src_frame_rate)

/*******************************************************
 *  Internal APIs
 ******************************************************/
void vo_fill_disp_timing(struct disp_timing *timing,
		struct vo_bt_timings *bt_timing)
{
	timing->vtotal = VO_DV_BT_FRAME_HEIGHT(bt_timing) - 1;
	timing->htotal = VO_DV_BT_FRAME_WIDTH(bt_timing) - 1;
	timing->vsync_start = 0;
	timing->vsync_end = timing->vsync_start + bt_timing->vsync - 1;
	timing->vfde_start = timing->vmde_start =
		timing->vsync_start + bt_timing->vsync + bt_timing->vbackporch;
	timing->vfde_end = timing->vmde_end =
		timing->vfde_start + bt_timing->height - 1;
	timing->hsync_start = 0;
	timing->hsync_end = timing->hsync_start + bt_timing->hsync - 1;
	timing->hfde_start = timing->hmde_start =
		timing->hsync_start + bt_timing->hsync + bt_timing->hbackporch;
	timing->hfde_end = timing->hmde_end =
		timing->hfde_start + bt_timing->width - 1;
	timing->vsync_pol = bt_timing->polarities & VO_DV_VSYNC_POS_POL;
	timing->hsync_pol = bt_timing->polarities & VO_DV_HSYNC_POS_POL;
}

static void disp_hw_enque(vo_dev dev, struct vo_layer_ctx *layer_ctx)
{
	// struct vo_buffer *buf;
	struct disp_buffer *now_buf = NULL;
	struct disp_buffer *next_buf = NULL;
	struct disp_buffer *done_buf = NULL;
	struct disp_cfg *cfg;
	// int i = 0;
	unsigned long flags;

	osal_spin_lock_irqsave(&layer_ctx->list_lock, &flags);

	if (osal_list_empty(&layer_ctx->list_wait) &&
	    osal_list_empty(&layer_ctx->list_work)) {
		osal_spin_unlock_irqrestore(&layer_ctx->list_lock, &flags);
		return;
	}

	if (!osal_list_empty(&layer_ctx->list_wait)) {
		if (!osal_list_empty(&layer_ctx->list_work)) {
			done_buf = osal_list_first_entry(&layer_ctx->list_work,
							struct disp_buffer, list);
			osal_list_move_tail(&done_buf->list, &layer_ctx->list_done);
		}

		next_buf = osal_list_first_entry(&layer_ctx->list_wait,
						 struct disp_buffer, list);
		osal_list_move_tail(&next_buf->list, &layer_ctx->list_work);
	}

	now_buf = osal_list_first_entry(&layer_ctx->list_work,
					struct disp_buffer, list);

	osal_spin_unlock_irqrestore(&layer_ctx->list_lock, &flags);

	if (now_buf == NULL || layer_ctx->chn_ctx[layer_ctx->bind_dev_id].pause)
		return;

	// buf = &now_buf->buf;
	// for (i = 0; i < 3; i++) {
	//	TRACE_VO(DBG_DEBUG, "now_buf->buf.planes[%d].addr=%lx\n",
	//		 i, now_buf->buf.planes[i].addr);
	// }

	disp_enable_window_bgcolor(dev, false);
	cfg = disp_get_cfg(dev);
	cfg->mem.start_x = now_buf->buf.start_x;
	cfg->mem.start_y = now_buf->buf.start_y;
	cfg->mem.addr0 = now_buf->buf.planes[0].addr;
	cfg->mem.addr1 = now_buf->buf.planes[1].addr;
	cfg->mem.addr2 = now_buf->buf.planes[2].addr;
	cfg->mem.pitch_y = now_buf->buf.planes[0].bytesused;
	cfg->mem.pitch_c = now_buf->buf.planes[1].bytesused;
	disp_set_mem(dev, &cfg->mem);

	layer_ctx->display_pts = ((struct vb_s *)(uintptr_t)now_buf->blk)->buf.pts;
}

int _vo_send_logo_from_ion(vo_layer layer, vo_chn chn,  video_frame_info_s *pstVideoFrame,
		int s32MilliSec)
{
	struct disp_cfg *cfg;
	unsigned int  u32Len;
	uint64_t u64PhyAddr;
	int S32Ret;
	UNUSED(chn);
	UNUSED(s32MilliSec);

	cfg = disp_get_cfg(layer);

	if(plogodata != NULL) {
		base_ion_free((uintptr_t)plogodata);
		TRACE_VO(DBG_INFO, "base_ion_free, [%p]\n", plogodata);
		plogodata = NULL;
	}
	u32Len = pstVideoFrame->video_frame.stride[0] * pstVideoFrame->video_frame.height * 3 / 2;//yuv420
	S32Ret = base_ion_alloc(&u64PhyAddr, &plogodata, (uint8_t *)"CVI_LOGO", u32Len, true);
	if (S32Ret != 0) {
		TRACE_VO(DBG_ERR, "base_ion_alloc failed.\n");
		return -1;
	}
	memcpy(plogodata, pstVideoFrame->video_frame.viraddr[0],
		pstVideoFrame->video_frame.stride[0] * pstVideoFrame->video_frame.height);
	memcpy(plogodata + pstVideoFrame->video_frame.stride[0] * pstVideoFrame->video_frame.height,
		pstVideoFrame->video_frame.viraddr[1],
		pstVideoFrame->video_frame.stride[1] * pstVideoFrame->video_frame.height / 2);
	base_ion_cache_flush(u64PhyAddr, plogodata, u32Len);

	disp_enable_window_bgcolor(layer, false);

	u64PhyAddr = (uintptr_t)plogodata;
	cfg->mem.addr0 = u64PhyAddr;
	cfg->mem.addr1 = u64PhyAddr +
			pstVideoFrame->video_frame.stride[0] * pstVideoFrame->video_frame.height;
	cfg->mem.addr2 = 0;
	cfg->mem.pitch_y = pstVideoFrame->video_frame.stride[0];
	cfg->mem.pitch_c = pstVideoFrame->video_frame.stride[1];
	// cfg->mem.start_x = pstVideoFrame->video_frame.stride[0] - pstVideoFrame->video_frame.u32Width;
	cfg->mem.start_x = 0;
	cfg->mem.start_y = 0;

	disp_set_mem(layer, &cfg->mem);
	return 0;
}

static int simplify_rate(unsigned int dst_in, unsigned int src_in,
			 unsigned int *dst_out, unsigned int *src_out)
{
	unsigned int i = 1;
	unsigned int a, b;

	while (i < dst_in + 1) {
		a = dst_in % i;
		b = src_in % i;
		if (a == 0 && b == 0) {
			dst_in = dst_in / i;
			src_in = src_in / i;
			i = 1;
		}
		i++;
	}

	*dst_out = dst_in;
	*src_out = src_in;

	return 0;
}

static unsigned char vo_frame_ctrl(unsigned long long frame_index, frame_rate_ctrl_s *frame_rate_ctrl)
{
	unsigned int src_simp;
	unsigned int dst_simp;
	unsigned int index;
	unsigned int src_dur, dst_dur;
	unsigned int cur_indx, next_indx;

	simplify_rate(frame_rate_ctrl->dst_frame_rate,
		      frame_rate_ctrl->src_frame_rate,
		      &dst_simp, &src_simp);

	index = osal_div_u64(frame_index, src_simp);
	if (index == 0)
		return true;

	src_dur = 100;
	dst_dur = (src_dur * src_simp) / dst_simp;
	cur_indx = (index - 1) * src_dur / dst_dur;
	next_indx = index * src_dur / dst_dur;

	if (next_indx == cur_indx)
		return false;

	return true;
}

static void _vo_gdc_callback(void *gdc_param, vb_blk blk)
{
	struct _vo_gdc_cb_param *cb_param = NULL;
	struct vo_layer_ctx *layer_ctx;
	struct vo_chn_ctx *chn_ctx;
	mmf_chn_s chn;
	struct vb_jobs_t *jobs;
	struct vb_s *vb = (struct vb_s *)(uintptr_t)blk;
	struct disp_buffer *disp_buf;
	unsigned char i = 0;
	unsigned long flags;

	if (!gdc_param)
		return;

	cb_param = (struct _vo_gdc_cb_param *)gdc_param;
	chn = cb_param->chn;
	layer_ctx = &g_vo_ctx->layer_ctx[chn.dev_id];
	chn_ctx = &layer_ctx->chn_ctx[chn.chn_id];

	if (!chn_ctx->is_chn_enable || g_vo_ctx->suspend) {
		TRACE_VO(DBG_INFO, "layer(%d) chn(%d) disable.\n", chn.dev_id, chn.chn_id);
		osal_atomic_set(&chn_ctx->mesh.gdc_flag, 0);
		vb_release_block(blk);
		osal_vfree(gdc_param);
		gdc_param = NULL;
		return;
	}

	if (chn_ctx->pause) {
		TRACE_VO(DBG_INFO, "layer(%d) chn(%d) pause.\n", chn.dev_id, chn.chn_id);
		osal_atomic_set(&chn_ctx->mesh.gdc_flag, 0);
		vb_release_block(blk);
		osal_vfree(gdc_param);
		gdc_param = NULL;
		return;
	}

	if (IS_VB_OFFSET_INVALID(vb->buf)) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) vb offset (%d %d %d %d) invalid\n",
			 chn.dev_id, chn.chn_id,
			 vb->buf.offset_left, vb->buf.offset_right,
			 vb->buf.offset_top, vb->buf.offset_bottom);
		osal_atomic_set(&chn_ctx->mesh.gdc_flag, 0);
		vb_release_block(blk);
		osal_vfree(gdc_param);
		gdc_param = NULL;
		return;
	}
	osal_atomic_set(&chn_ctx->mesh.gdc_flag, 0);

	disp_buf = osal_vzalloc(sizeof(*disp_buf));
	if (!disp_buf) {
		TRACE_VO(DBG_ERR, "osal_vzalloc size(%zu) fail\n", sizeof(struct disp_buffer));
		return;
	}

	osal_mutex_lock(&layer_ctx->layer_lock);

	jobs = &chn_ctx->chn_jobs;
	osal_mutex_lock(&jobs->lock);
	if (!jobs->inited) {
		osal_mutex_unlock(&jobs->lock);
		osal_mutex_unlock(&layer_ctx->layer_lock);
		osal_vfree(disp_buf);
		TRACE_VO(DBG_NOTICE, "layer(%d) chn(%d) jobs not initialized yet.\n",
				chn.dev_id, chn.chn_id);
		return;
	}
	if (FIFO_FULL(&jobs->waitq)) {
		struct vb_s *vb_old = NULL;
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) waitq full, drop frame.\n",
			     chn.dev_id, chn.chn_id);
		FIFO_POP(&jobs->waitq, &vb_old);
		osal_atomic_fetch_and(~BIT(chn.mod_id), &vb_old->mod_ids);
		vb_release_block((vb_blk)(uintptr_t)vb_old);
	}

	chn_ctx->frame_num++;
	layer_ctx->frame_num++;

	FIFO_PUSH(&jobs->waitq, vb);
	osal_mutex_unlock(&jobs->lock);
	TRACE_VO(DBG_INFO, "layer(%d) chn(%d) push vb(0x%llx).\n",
		 chn.dev_id, chn.chn_id, vb->phy_addr);

	osal_atomic_fetch_or(BIT(chn.mod_id), &vb->mod_ids);
	osal_vfree(gdc_param);

	disp_buf->blk = (vb_blk)(uintptr_t)vb;
	disp_buf->buf.length = 3;

	disp_buf->buf.start_x = vb->buf.offset_left;
	disp_buf->buf.start_y = vb->buf.offset_top;

	for (i = 0; i < disp_buf->buf.length; i++) {
		disp_buf->buf.planes[i].addr = vb->buf.phy_addr[i];
		disp_buf->buf.planes[i].bytesused = vb->buf.stride[i];
	}

	osal_spin_lock_irqsave(&layer_ctx->list_lock, &flags);
	osal_list_add_tail(&disp_buf->list, &layer_ctx->list_wait);
	osal_spin_unlock_irqrestore(&layer_ctx->list_lock, &flags);

	osal_mutex_unlock(&layer_ctx->layer_lock);

	TRACE_VO(DBG_INFO, "layer(%d) add buffer(0x%llx) to wait list.\n",
		 layer_ctx->layer, vb->phy_addr);
}

static int _mesh_gdc_do_op_cb(enum gdc_usage usage, const void *usage_param,
			      struct vb_s *vb_in, pixel_format_e pixformat, unsigned long long mesh_addr,
			      unsigned char sync_io, void *cb_param, unsigned int cb_param_size,
			      mod_id_e mod_id, rotation_e rotation)
{
	struct mesh_gdc_cfg cfg;
	struct base_exe_m_cb exe_cb;

	memset(&cfg, 0, sizeof(cfg));
	cfg.usage = usage;
	cfg.usage_param = usage_param;
	cfg.vb_in = vb_in;
	cfg.pix_format = pixformat;
	cfg.mesh_addr = mesh_addr;
	cfg.sync_io = sync_io;
	cfg.cb_param = cb_param;
	cfg.cb_param_size = cb_param_size;
	cfg.rotation = rotation;

	exe_cb.callee = E_MODULE_LDC;
	exe_cb.caller = E_MODULE_VO;
	exe_cb.cmd_id = LDC_CB_MESH_GDC_OP;
	exe_cb.data   = &cfg;
	return base_exe_module_cb(&exe_cb);
}

static int vo_get_chn_buffers(struct vo_layer_ctx *layer_ctx, vb_blk *blk)
{
	vo_chn chn;
	struct vo_chn_ctx *chn_ctx;
	struct vb_jobs_t *jobs;
	struct vb_s *old_workq = NULL;
	struct vb_s *new_workq = NULL;
	struct vb_s *vb;
	int chn_num = 0;

	for (chn = 0; chn < VO_MAX_CHN_NUM; ++chn) {
		chn_ctx = &layer_ctx->chn_ctx[chn];
		jobs = &chn_ctx->chn_jobs;

		if (!chn_ctx->is_chn_enable)
			continue;

		if (chn_ctx->pause) {
			osal_mutex_lock(&jobs->lock);
			if (FIFO_EMPTY(&jobs->workq)) {
				osal_mutex_unlock(&jobs->lock);
				continue;
			}
			while (FIFO_SIZE(&jobs->workq) != 1) {
				FIFO_POP(&jobs->workq, &vb);
				vb_release_block((vb_blk)(uintptr_t)vb);
			}
			FIFO_GET_FRONT(&jobs->workq, &vb);
			blk[chn] = (vb_blk)(uintptr_t)vb;
			chn_num++;
			osal_mutex_unlock(&jobs->lock);
			continue;
		}

		osal_mutex_lock(&jobs->lock);
		if (FIFO_EMPTY(&jobs->workq) && FIFO_EMPTY(&jobs->waitq)) {
			osal_mutex_unlock(&jobs->lock);
			continue;
		}

		if (!FIFO_EMPTY(&jobs->waitq)) {
			if (!FIFO_EMPTY(&jobs->workq))
				FIFO_POP(&jobs->workq, &old_workq);

			FIFO_POP(&jobs->waitq, &vb);
			FIFO_PUSH(&jobs->workq, vb);
		}
		osal_mutex_unlock(&jobs->lock);

		FIFO_GET_FRONT(&jobs->workq, &new_workq);
		blk[chn] = (vb_blk)(uintptr_t)new_workq;
		if (old_workq) {
			chn_ctx->predone_pts = old_workq->buf.pts;
			vb_release_block((vb_blk)(uintptr_t)old_workq);
			old_workq = NULL;
		}
		chn_num++;
	}

	return chn_num;
}

static int layer_process(struct vo_layer_ctx *layer_ctx)
{
	struct vo_chn_ctx *chn_ctx;
	unsigned char i = 0;
	unsigned int chn_num;
	struct vb_s *vb;
	vb_blk blk_out = VB_INVALID_HANDLE;
	vb_blk blks[VO_MAX_CHN_NUM] = { [0 ... VO_MAX_CHN_NUM - 1] = VB_INVALID_HANDLE };
	struct disp_buffer *disp_buf;
	unsigned long flags;

	if (!osal_list_empty(&layer_ctx->list_done)) {
		osal_spin_lock_irqsave(&layer_ctx->list_lock, &flags);
		disp_buf = osal_list_first_entry(&layer_ctx->list_done, struct disp_buffer, list);
		osal_list_del_init(&disp_buf->list);
		osal_spin_unlock_irqrestore(&layer_ctx->list_lock, &flags);

		blk_out = disp_buf->blk;
		vb = (struct vb_s *)(uintptr_t)blk_out;
		layer_ctx->predone_pts = vb->buf.pts;
		layer_ctx->done_cnt++;

		osal_vfree(disp_buf);
	}

	osal_mutex_lock(&layer_ctx->layer_lock);
	chn_num = vo_get_chn_buffers(layer_ctx, blks);
	if (!chn_num) {
		osal_mutex_unlock(&layer_ctx->layer_lock);
		return 0;
	}

	for (i = 0; i < chn_num; i++) {
		vb = (struct vb_s *)(uintptr_t)blks[i];
		chn_ctx = &layer_ctx->chn_ctx[i];

		TRACE_VO(DBG_INFO, "layer(%d) chn(%d) show vb(0x%llx) size(%d, %d) frm_num(%u).\n",
			 layer_ctx->layer, i, vb->phy_addr,
			 vb->buf.size.width, vb->buf.size.height,
			 vb->buf.frm_num);

		chn_ctx->display_pts = vb->buf.pts;
		chn_ctx->src_width = vb->buf.size.width;
		chn_ctx->src_height = vb->buf.size.height;
	}

	osal_mutex_unlock(&layer_ctx->layer_lock);

	if(plogodata != NULL) {
		base_ion_free((uintptr_t)plogodata);
		TRACE_VO(DBG_INFO, "base_ion_free, [%p]\n", plogodata);
		plogodata = NULL;
	}

	return 0;
}

int vb_wait_condition(const void *param)
{
	struct vo_layer_ctx *layer_ctx = (struct vo_layer_ctx *)param;

	return layer_ctx->event;
}

static int disp_event_handler(void *arg)
{
	struct vo_layer_ctx *layer_ctx = (struct vo_layer_ctx *)arg;
	unsigned long timeout = WAIT_TIMEOUT_MS;
	int ret;

	while (!osal_kthread_should_stop()) {
		ret = osal_wait_timeout_interruptible(&layer_ctx->wq,
							    vb_wait_condition,
							    layer_ctx,
							    timeout);

		/* -%ERESTARTSYS */
		if (ret < 0 || osal_kthread_should_stop())
			break;

		/* timeout */
		if (!ret)
			continue;

		//TRACE_VO(DBG_INFO, "[%d] thread run.\n", layer_ctx->layer);
		layer_ctx->event = 0;
		layer_process(layer_ctx);
	}

	return 0;
}

int vo_create_thread(vo_layer layer)
{
	char task_name[32];
	struct vo_layer_ctx *layer_ctx = &g_vo_ctx->layer_ctx[layer];

	snprintf(task_name, sizeof(task_name), "task_vo_layer%d", layer);

	layer_ctx->thread = osal_kthread_create(disp_event_handler, (void *)layer_ctx,
						task_name, 0);
	if (layer_ctx->thread == NULL) {
		TRACE_VO(DBG_ERR, "Unable to create %s.\n", task_name);
		return -1;
	}
	osal_kthread_set_priority(layer_ctx->thread, OSAL_TASK_PRIORITY_HIGH);

	TRACE_VO(DBG_INFO, "[layer%d]create thread.\n", layer);

	return 0;
}

int vo_destroy_thread(vo_layer layer)
{
	struct vo_layer_ctx *layer_ctx = &g_vo_ctx->layer_ctx[layer];

	osal_kthread_destroy(layer_ctx->thread, 1);

	layer_ctx->thread = NULL;
	disp_enable_window_bgcolor(layer, true);
	TRACE_VO(DBG_INFO, "[layer%d]destroy thread.\n", layer);

	return 0;
}

/*******************************************************
 *  Irq handlers
 ******************************************************/
static void irq_handler(struct vo_dev_ctx *dev_ctx, union disp_intr intr_status)
{
	vo_dev dev_id = dev_ctx->dev_id;
	union disp_dbg_status status = disp_get_dbg_status(dev_id, true);
	vo_layer layer_id = dev_ctx->bind_layer_id;
	struct vo_layer_ctx *layer_ctx;

	if (osal_atomic_read(&dev_ctx->disp_streamon) == 0)
		return;

	if (intr_status.b.disp_frame_end) {
		++dev_ctx->frame_number;

		layer_ctx = &g_vo_ctx->layer_ctx[layer_id];
		if (!layer_ctx->is_layer_enable)
			return;

		if (status.b.bw_fail) {
			layer_ctx->bw_fail++;
			TRACE_VO(DBG_WARN, "dev(%d) disp bw failed at frame#%d\n", dev_id, dev_ctx->frame_number);
		}

		if (status.b.osd_bw_fail) {
			layer_ctx->vgop_bw_fail++;
			TRACE_VO(DBG_WARN, "dev(%d) osd bw failed at frame#%d\n", dev_id, dev_ctx->frame_number);
		}

		if (!dev_ctx->disp_online) {
			disp_hw_enque(dev_id, layer_ctx);
			layer_ctx->event = 1;
			osal_wait_wakeup(&layer_ctx->wq);
		}
	}
}

int vo_call_gfbg(u32 m_id, u32 cmd_id, void *data)
{
	struct base_exe_m_cb exe_cb;

	exe_cb.callee = m_id;
	exe_cb.caller = E_MODULE_VO;
	exe_cb.cmd_id = cmd_id;
	exe_cb.data   = (void *)data;

	return base_exe_module_cb(&exe_cb);
}

int vo_irq_handler(int irq, void *data)
{
	struct vo_dev_ctx *dev_ctx = (struct vo_dev_ctx *)data;
	union disp_intr intr_status;
	union disp_intr_clr intr_clr;
	vo_dev dev_id = dev_ctx->dev_id;
#if defined(__KERNEL__)
	struct gfbg_int_status gfbg_int;
#endif
	int i;

	if (dev_ctx->irq_num != irq) {
		TRACE_VO(DBG_ERR, "irq(%d) Error.\n", irq);
		return 1;
	}

	intr_status = disp_intr_status(dev_id);
	intr_clr.b.disp_frame_end = intr_status.b.disp_frame_end;
	intr_clr.b.disp_frame_start = intr_status.b.disp_frame_start;

	disp_intr_clr(dev_id, intr_clr);

	irq_handler(dev_ctx, intr_status);

	//call gfbg
	for (i = 0; i < VO_MAX_GRAPHIC_LAYER_NUM; ++i) {
		if (g_vo_ctx->overlay_ctx[i].enable &&
		    g_vo_ctx->overlay_ctx[i].bind_dev_id == dev_id) {
#if defined(__KERNEL__)
			gfbg_int.dev_id = dev_id;
			gfbg_int.layer_id = i;
			gfbg_int.intr_status = intr_status;
			vo_call_gfbg(E_MODULE_GFBG, GFBG_CB_IRQ_HANDLER, &gfbg_int);
#endif
		}
	}

	return 1;
}

int vo_process_init_ctx(struct vo_ctx *ctx)
{
	int ret = 0, i, j;
	unsigned short rgb[3] = {0, 0, 0};

	for (i = 0; i < VO_MAX_DEV_NUM; ++i) {
		ctx->dev_ctx[i].dev_id = i;
		ctx->dev_ctx[i].disp_online = false;
		ctx->dev_ctx[i].bind_layer_id = i;
		if (hide_vo) {
			disp_set_pattern(i, PAT_TYPE_FULL, PAT_COLOR_USR, rgb);
			disp_set_frame_bgcolor(i, 0, 0, 0);
		}
	}

	for (i = 0; i < VO_MAX_VIDEO_LAYER_NUM; ++i) {
		ctx->layer_ctx[i].bind_dev_id = i;
		ctx->layer_ctx[i].layer = i;
		ctx->layer_ctx[i].display_buflen = 2;

		osal_spin_lock_init(&ctx->layer_ctx[i].list_lock);
		OSAL_INIT_LIST_HEAD(&ctx->layer_ctx[i].list_wait);
		OSAL_INIT_LIST_HEAD(&ctx->layer_ctx[i].list_work);
		OSAL_INIT_LIST_HEAD(&ctx->layer_ctx[i].list_done);
		osal_mutex_init(&ctx->layer_ctx[i].layer_lock);

		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			osal_atomic_set(&ctx->layer_ctx[i].chn_ctx[j].mesh.gdc_flag, 0);
		}
	}

	for (i = 0; i < VO_MAX_DEV_NUM; ++i) {
		for (j = 0; j < VO_MAX_GRAPHIC_LAYER_IN_DEV; ++j) {
			ctx->dev_ctx[i].bind_overlay_id[j] = default_overlay_bind_priority[i][j];
			ctx->overlay_ctx[default_overlay_bind_priority[i][j] -
				VO_MAX_VIDEO_LAYER_NUM].bind_dev_id = i;
			ctx->overlay_ctx[default_overlay_bind_priority[i][j] -
				VO_MAX_VIDEO_LAYER_NUM].priority = 0;
		}
	}

	for (i = 0; i < VO_MAX_GRAPHIC_LAYER_NUM; ++i) {
		ctx->overlay_ctx[i].enable = false;
	}

	return ret;
}

int vo_process_deinit_ctx(struct vo_ctx *ctx)
{
	int ret = 0, i;

	for (i = 0; i < VO_MAX_VIDEO_LAYER_NUM; ++i) {
		osal_spin_lock_destroy(&ctx->layer_ctx[i].list_lock);
		osal_mutex_destroy(&ctx->layer_ctx[i].layer_lock);
	}

	return ret;
}

int vo_recv_frame(mmf_chn_s chn, vb_blk blk, void *data)
{
	int ret;
	struct vo_layer_ctx *layer_ctx;
	struct vo_chn_ctx *chn_ctx;
	struct vb_jobs_t *jobs;
	struct vb_s *vb = (struct vb_s *)(uintptr_t)blk;
	frame_rate_ctrl_s chn_frame_ctrl;
	struct disp_buffer *disp_buf;
	unsigned char i = 0;
	unsigned long flags;
	size_s size;

	ret = check_vo_chn_valid(chn.dev_id, chn.chn_id);
	if (ret != 0)
		return ret;

	if (g_vo_ctx->suspend) {
		TRACE_VO(DBG_WARN, "vo suspend do not recv frame.\n");
		return -1;
	}

	layer_ctx = &g_vo_ctx->layer_ctx[chn.dev_id];
	chn_ctx = &layer_ctx->chn_ctx[chn.chn_id];

	if (!chn_ctx->is_chn_enable) {
		TRACE_VO(DBG_WARN, "layer(%d) chn(%d) disable.\n", chn.dev_id, chn.chn_id);
		return -1;
	}

	chn_ctx->frame_index++;
	chn_ctx->src_frame_num++;
	layer_ctx->frame_index++;
	layer_ctx->src_frame_num++;
	chn_frame_ctrl.src_frame_rate = chn_ctx->src_frame_rate;
	chn_frame_ctrl.dst_frame_rate = chn_ctx->frame_rate_user_set;
	chn_ctx->is_drop = false;

	if (!FRC_INVALID(chn_frame_ctrl) && (!vo_frame_ctrl(chn_ctx->frame_index, &chn_frame_ctrl)))
		chn_ctx->is_drop = true;

	if (chn_ctx->pause || chn_ctx->is_drop)
		return 0;

	if (layer_ctx->layer_attr.pixformat != vb->buf.pixel_format) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) PixelFormat(%d) mismatch.\n",
			 chn.dev_id, chn.chn_id, vb->buf.pixel_format);
		return -1;
	}

	if ((chn_ctx->rotation == ROTATION_90) || (chn_ctx->rotation == ROTATION_270)) {
		size.width = layer_ctx->layer_attr.img_size.height;
		size.height = layer_ctx->layer_attr.img_size.width;
	} else
		size = layer_ctx->layer_attr.img_size;

	if ((size.width != (vb->buf.size.width -
		vb->buf.offset_left - vb->buf.offset_right)) ||
		(size.height != (vb->buf.size.height -
		vb->buf.offset_top - vb->buf.offset_bottom))) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) size(%d * %d) mismatch.\n",
			 chn.dev_id, chn.chn_id, vb->buf.size.width, vb->buf.size.height);
		return -1;
	}

	if (IS_VB_OFFSET_INVALID(vb->buf)) {
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) vb offset (%d %d %d %d) invalid\n",
			 chn.dev_id, chn.chn_id,
			 vb->buf.offset_left, vb->buf.offset_right,
			 vb->buf.offset_top, vb->buf.offset_bottom);
		return ERR_VO_ILLEGAL_PARAM;
	}

	if (chn_ctx->rotation != ROTATION_0) {
		if (!osal_atomic_cmpxchg(&chn_ctx->mesh.gdc_flag, 0, 1)) {
			struct _vo_gdc_cb_param cb_param = { .chn = chn, .usage = GDC_USAGE_LDC};

			osal_atomic_inc_return(&vb->usr_cnt);
			osal_atomic_fetch_or(BIT(ID_GDC), &vb->mod_ids);

			if (_mesh_gdc_do_op_cb(GDC_USAGE_ROTATION
				, NULL
				, vb
				, g_vo_ctx->layer_ctx[chn.dev_id].layer_attr.pixformat
				, chn_ctx->mesh.paddr
				, false, &cb_param
				, sizeof(cb_param)
				, ID_VO
				, chn_ctx->rotation) != 0) {
				osal_atomic_set(&chn_ctx->mesh.gdc_flag, 0);
				TRACE_VO(DBG_ERR, "gdc rotation failed.\n");
				return -1;
			}

			TRACE_VO(DBG_DEBUG, "layer(%d) chn(%d) push vb(0x%llx) to gdc job.\n",
				 chn.dev_id, chn.chn_id, vb->phy_addr);

			return 0;
		}

		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) drop frame due to gdc op blocked.\n",
			 chn.dev_id, chn.chn_id);
		return -1;
	}

	disp_buf = osal_vzalloc(sizeof(*disp_buf));
	if (!disp_buf) {
		TRACE_VO(DBG_ERR, "osal_vzalloc size(%zu) fail\n", sizeof(struct disp_buffer));
		ret = ERR_VO_NO_MEM;
		return ret;
	}

	osal_mutex_lock(&layer_ctx->layer_lock);

	jobs = &chn_ctx->chn_jobs;
	osal_mutex_lock(&jobs->lock);
	if (!jobs->inited) {
		osal_mutex_unlock(&jobs->lock);
		osal_mutex_unlock(&layer_ctx->layer_lock);
		TRACE_VO(DBG_NOTICE, "layer(%d) chn(%d) jobs not initialized yet.\n",
				chn.dev_id, chn.chn_id);
		osal_vfree(disp_buf);
		return -1;
	}
	if (FIFO_FULL(&jobs->waitq)) {
		struct vb_s *vb_old = NULL;
		TRACE_VO(DBG_ERR, "layer(%d) chn(%d) waitq full, drop frame.\n",
			     chn.dev_id, chn.chn_id);
		FIFO_POP(&jobs->waitq, &vb_old);
		osal_atomic_fetch_and(~BIT(chn.mod_id), &vb_old->mod_ids);
		vb_release_block((vb_blk)(uintptr_t)vb_old);
	}

	chn_ctx->frame_num++;
	layer_ctx->frame_num++;

	FIFO_PUSH(&jobs->waitq, vb);
	osal_mutex_unlock(&jobs->lock);

	osal_atomic_inc_return(&vb->usr_cnt);
	osal_atomic_fetch_or(BIT(chn.mod_id), &vb->mod_ids);

	TRACE_VO(DBG_INFO, "layer(%d) chn(%d) push vb(0x%llx).\n",
		 chn.dev_id, chn.chn_id, vb->phy_addr);

	disp_buf->blk = (vb_blk)(uintptr_t)vb;
	disp_buf->buf.length = 3;

	disp_buf->buf.start_x = vb->buf.offset_left;
	disp_buf->buf.start_y = vb->buf.offset_top;

	for (i = 0; i < disp_buf->buf.length; i++) {
		disp_buf->buf.planes[i].addr = vb->buf.phy_addr[i];
		disp_buf->buf.planes[i].bytesused = vb->buf.stride[i];
	}

	osal_spin_lock_irqsave(&layer_ctx->list_lock, &flags);
	osal_list_add_tail(&disp_buf->list, &layer_ctx->list_wait);
	osal_spin_unlock_irqrestore(&layer_ctx->list_lock, &flags);

	osal_mutex_unlock(&layer_ctx->layer_lock);

	TRACE_VO(DBG_INFO, "layer(%d) add buffer(0x%llx) to wait list.\n",
		 layer_ctx->layer, vb->phy_addr);

	return ret;
}

int vo_cb(void *dev, cb_modules_id caller, unsigned int cmd, void *arg)
{
	int rc = -1;
	(void)dev;
	switch (cmd) {
	case VO_CB_GDC_OP_DONE:
	{
		struct ldc_op_done_cfg *cfg = (struct ldc_op_done_cfg *)arg;

		_vo_gdc_callback(cfg->param, cfg->blk);
		rc = 0;
		break;
	}

	default:
		break;
	}

	return rc;
}

int vo_core_cb(void *dev, cb_modules_id caller, unsigned int cmd, void *arg)
{
	return vo_cb(dev, caller, cmd, arg);
}

int vo_core_register_cb(void *dev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_VO;
	reg_cb.dev		= (void *)dev;
	reg_cb.cb		= vo_core_cb;

	return base_reg_module_cb(&reg_cb);
}

int vo_core_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_VO);
}

int vo_start_streaming(vo_dev dev)
{
	int rc = 0;
	struct vo_dev_ctx *dev_ctx;
	rc = check_vo_dev_valid(dev);
	if (rc != 0)
		return rc;

	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	disp_enable_window_bgcolor(dev, true);

	dev_ctx->frame_number = 0;

	disp_tgen_enable(dev, true);

	osal_atomic_set(&dev_ctx->disp_streamon, 1);
	TRACE_VO(DBG_INFO, "[dev%d]start streaming.\n", dev);
	rc = osal_irq_request(dev_ctx->irq_num, vo_irq_handler, 0,
			      disp_irq_name[dev], (void *)dev_ctx);
	if (rc) {
		TRACE_VO(DBG_ERR, "Failed to vo request irq\n");
		return rc;
	}

	return rc;
}

int vo_stop_streaming(vo_dev dev)
{
	int rc = 0;
	struct vo_dev_ctx *dev_ctx;
	rc = check_vo_dev_valid(dev);
	if (rc != 0)
		return rc;

	dev_ctx = &g_vo_ctx->dev_ctx[dev];

	disp_enable_window_bgcolor(dev, true);
	disp_tgen_enable(dev, false);

	osal_atomic_set(&dev_ctx->disp_streamon, 0);
	TRACE_VO(DBG_INFO, "[dev%d]stop streaming.\n", dev);
	TRACE_VO(DBG_DEBUG, "end...\n");

	osal_irq_free(dev_ctx->irq_num, (void *)dev_ctx);

	return rc;
}
