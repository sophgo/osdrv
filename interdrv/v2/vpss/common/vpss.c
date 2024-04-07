#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <asm/div64.h>

#include <linux/cvi_base_ctx.h>
#include <linux/cvi_defines.h>
#include <linux/cvi_common.h>
#include <linux/cvi_vip.h>
#include <linux/cvi_buffer.h>

#include "base_common.h"
#include "ion.h"
#include "sys.h"
#include "bind.h"
#include <vb.h>
#include "vbq.h"
#include <base_cb.h>
#include <vpss_cb.h>
#include <ldc_cb.h>
#include <vcodec_cb.h>
#include "vi_sys.h"
#include "vpss.h"
#include "vpss_common.h"
#include "vpss_hal.h"
#include "vpss_rgn_ctrl.h"

/*******************************************************
 *  MACRO definition
 ******************************************************/
#ifndef FPGA_PORTING
#define IDLE_TIMEOUT_MS      10000
#define EOF_WAIT_TIMEOUT_MS  1000
#else
#define IDLE_TIMEOUT_MS      60000
#define EOF_WAIT_TIMEOUT_MS  60000
#endif

#define CTX_EVENT_WKUP       0x0001
#define CTX_EVENT_EOF        0x0002
#define CTX_EVENT_VI_ERR     0x0004

#define EXT_POOL             0xffff
#define EXT_POOL_IN          0
#define EXT_POOL_OUT         1

#define BM_FIFO_INIT(head, _capacity, _fifo) do {		\
		if (_capacity > 0)						\
		(head)->fifo = _fifo;	\
		(head)->front = (head)->tail = -1;				\
		(head)->capacity = _capacity;					\
	} while (0)

#define BM_FIFO_EXIT(head) do {						\
		(head)->front = (head)->tail = -1;			\
		(head)->capacity = 0; 					\
		(head)->fifo = NULL;					\
	} while (0)

struct vpss_jobs_ctx {
	struct vb_jobs_t ins;
	struct vb_jobs_t outs[VPSS_MAX_CHN_NUM];
};

struct vpss_handler_ctx {
	wait_queue_head_t wait;
	struct task_struct *thread;
	spinlock_t hdl_lock;
	u64 GrpMask;
	u8 events;
};

struct _vpss_gdc_cb_param {
	MMF_CHN_S chn;
	enum GDC_USAGE usage;
};

struct vpss_ext_ctx {
	struct cvi_csc_cfg csc_cfg;
	struct cvi_csc_cfg chn_csc_cfg[VPSS_MAX_CHN_NUM];
	s32 proc_amp[PROC_AMP_MAX];
};

struct vpss_stitch_data {
	wait_queue_head_t wait;
	u8 u8Flag;
};

static struct cvi_vpss_ctx *vpssCtx[VPSS_MAX_GRP_NUM] = { [0 ... VPSS_MAX_GRP_NUM - 1] = NULL };

// cvi_vpss_ctx in uapi, internal extension version.
static struct vpss_ext_ctx vpssExtCtx[VPSS_MAX_GRP_NUM];

static struct vpss_jobs_ctx stVpssVbjobs[VPSS_MAX_GRP_NUM];

struct cvi_vpss_job job_bm[VPSS_MAX_GRP_NUM][2];

static struct vb_s vb_bm[VPSS_MAX_GRP_NUM][7];

static u8 is_bm_scene = false;

// Motion level for vcodec
static struct mlv_i_s g_mlv_i[VI_MAX_DEV_NUM];

static struct workqueue_struct *vpss_workqueue;

//update proc info
static void _update_vpss_chn_real_frame_rate(struct timer_list *timer);
DEFINE_TIMER(timer_proc, _update_vpss_chn_real_frame_rate);

//vpss job prepare
static struct vpss_handler_ctx vpss_hdl_ctx;

//global lock
static struct mutex g_VpssLock;

//Get Available Grp lock
static u8 s_VpssGrpUsed[VPSS_MAX_GRP_NUM];

//timer callback
static vpss_timer_cb s_core_cb;
static void *s_core_data;

static struct cvi_gdc_mesh mesh[VPSS_MAX_GRP_NUM][VPSS_MAX_CHN_NUM];

static PROC_AMP_CTRL_S procamp_ctrls[PROC_AMP_MAX] = {
	{ .minimum = 0, .maximum = 100, .step = 1, .default_value = 50 },
	{ .minimum = 0, .maximum = 100, .step = 1, .default_value = 50 },
	{ .minimum = 0, .maximum = 100, .step = 1, .default_value = 50 },
	{ .minimum = 0, .maximum = 100, .step = 1, .default_value = 50 },
};

static VI_VPSS_MODE_S stVIVPSSMode;
static VPSS_MOD_PARAM_S stModParam;

static void bm_base_mod_jobs_init(VPSS_GRP grp, uint8_t vb_idx, struct vb_jobs_t *jobs, uint8_t waitq_depth, uint8_t workq_depth, uint8_t doneq_depth)
{
	if (jobs == NULL) {
		pr_err("[%p] job init fail, Null parameter\n", __builtin_return_address(0));
		return;
	}

	if (jobs->inited) {
		pr_err("[%p] job init fail, already inited\n", __builtin_return_address(0));
		return;
	}

	mutex_init(&jobs->lock);
	mutex_init(&jobs->dlock);
	sema_init(&jobs->sem, 0);
	BM_FIFO_INIT(&jobs->waitq, waitq_depth, (struct vb_s**)&vb_bm[grp][vb_idx]);
	BM_FIFO_INIT(&jobs->workq, workq_depth, (struct vb_s**)&vb_bm[grp][vb_idx + 1]);
	BM_FIFO_INIT(&jobs->doneq, doneq_depth, (struct vb_s**)&vb_bm[grp][vb_idx + 2]);
	TAILQ_INIT(&jobs->snap_jobs);
	jobs->inited = true;
}

static void bm_base_mod_jobs_exit(struct vb_jobs_t *jobs)
{
	struct vb_s *vb;
	struct snap_s *s, *s_tmp;

	if (jobs == NULL) {
		pr_err("[%p] job exit fail, Null parameter\n", __builtin_return_address(0));
		return;
	}

	if (!jobs->inited) {
		pr_err("[%p] job exit fail, not inited yet\n", __builtin_return_address(0));
		return;
	}

	mutex_lock(&jobs->lock);
	while (!FIFO_EMPTY(&jobs->waitq)) {
		FIFO_POP(&jobs->waitq, &vb);
		vb_release_block((VB_BLK)vb);
	}
	BM_FIFO_EXIT(&jobs->waitq);
	while (!FIFO_EMPTY(&jobs->workq)) {
		FIFO_POP(&jobs->workq, &vb);
		vb_release_block((VB_BLK)vb);
	}
	BM_FIFO_EXIT(&jobs->workq);
	mutex_unlock(&jobs->lock);
	mutex_destroy(&jobs->lock);

	mutex_lock(&jobs->dlock);
	while (!FIFO_EMPTY(&jobs->doneq)) {
		FIFO_POP(&jobs->doneq, &vb);
		vb_release_block((VB_BLK)vb);
	}
	BM_FIFO_EXIT(&jobs->doneq);

	TAILQ_FOREACH_SAFE(s, &jobs->snap_jobs, tailq, s_tmp)
	TAILQ_REMOVE(&jobs->snap_jobs, s, tailq);
	mutex_unlock(&jobs->dlock);
	mutex_destroy(&jobs->dlock);
	jobs->inited = false;
}

static inline s32 check_vpss_grp_created(VPSS_GRP grp)
{
	if (!vpssCtx[grp] || !vpssCtx[grp]->isCreated) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) isn't created yet.\n", grp);
		return CVI_ERR_VPSS_UNEXIST;
	}
	return CVI_SUCCESS;
}

static inline s32 check_vpss_chn_valid(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{

	if ((VpssChn >= VPSS_MAX_CHN_NUM) || (VpssChn < 0)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) invalid channel ID.\n", VpssGrp, VpssChn);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	return CVI_SUCCESS;
}

s32 check_vpss_id(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
	s32 ret = CVI_SUCCESS;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;
	return ret;
}

void vpss_notify_wkup_evt(void)
{
	spin_lock(&vpss_hdl_ctx.hdl_lock);
	vpss_hdl_ctx.events |= CTX_EVENT_WKUP;
	spin_unlock(&vpss_hdl_ctx.hdl_lock);
	wake_up_interruptible(&vpss_hdl_ctx.wait);
}

void vpss_wkup_frame_done_handle(void *pdata)
{
	struct cvi_vpss_job *pstJob = container_of(pdata, struct cvi_vpss_job, data);

	queue_work(vpss_workqueue, &pstJob->job_work);
}

struct cvi_vpss_ctx **vpss_get_ctx(void)
{
	return vpssCtx;
}

void vpss_get_mod_param_void(VPSS_MOD_PARAM_S *pstModParam)
{
	*pstModParam = stModParam;
}

static u8 vpss_online_is_idle(void)
{
	int i;

	for (i = 0; i < VPSS_ONLINE_NUM; i++)
		if (vpssCtx[i] && vpssCtx[i]->isCreated && vpssCtx[i]->isStarted)
			return CVI_TRUE;

	return CVI_FALSE;
}

static s32 _mesh_gdc_do_op_cb(enum GDC_USAGE usage, const void *pUsageParam,
				struct vb_s *vb_in, PIXEL_FORMAT_E enPixFormat, u64 mesh_addr,
				u8 sync_io, void *pcbParam, u32 cbParamSize,
				MOD_ID_E enModId, ROTATION_E enRotation)
{
	struct mesh_gdc_cfg cfg;
	struct base_exe_m_cb exe_cb;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "push jobs(%d) for gdc\n", usage);

	memset(&cfg, 0, sizeof(cfg));
	cfg.usage = usage;
	cfg.pUsageParam = pUsageParam;
	cfg.vb_in = vb_in;
	cfg.enPixFormat = enPixFormat;
	cfg.mesh_addr = mesh_addr;
	cfg.sync_io = sync_io;
	cfg.pcbParam = pcbParam;
	cfg.cbParamSize = cbParamSize;
	cfg.enRotation = enRotation;

	exe_cb.callee = E_MODULE_LDC;
	exe_cb.caller = E_MODULE_VPSS;
	exe_cb.cmd_id = LDC_CB_MESH_GDC_OP;
	exe_cb.data   = &cfg;

	if(usage == GDC_USAGE_FISHEYE)
		exe_cb.callee = E_MODULE_DWA;
	else {
		LDC_ATTR_S *pstAttr = (LDC_ATTR_S *)pUsageParam;
		if(pstAttr && !pstAttr->bEnHWLDC)
			exe_cb.callee = E_MODULE_DWA;
	}

	return base_exe_module_cb(&exe_cb);
}

/* aspect_ratio_resize: calculate the new rect to keep aspect ratio
 *   according to given in/out size.
 *
 * @param in: input video size.
 * @param out: output display size.
 *
 * @return: the rect which describe the video on output display.
 */
static RECT_S aspect_ratio_resize(SIZE_S in, SIZE_S out)
{
	RECT_S rect;
	u32 scale = in.u32Height * in.u32Width;
	u32 ratio_int = MIN2(out.u32Width * in.u32Height, out.u32Height * in.u32Width);
	u64 height, width;

	//float ratio = MIN2((float)out.u32Width / in.u32Width, (float)out.u32Height / in.u32Height);
	//rect.u32Height = (float)in.u32Height * ratio + 0.5;
	//rect.u32Width = (float)in.u32Width * ratio + 0.5;
	//rect.s32X = (out.u32Width - rect.u32Width) >> 1;
	//rect.s32Y = (out.u32Height - rect.u32Height) >> 1;

	height = (u64)in.u32Height * ratio_int + scale/2;
	do_div(height, scale);
	rect.u32Height = (u32)height;

	width = (u64)in.u32Width * ratio_int + scale/2;
	do_div(width, scale);
	rect.u32Width = (u32)width;

	rect.s32X = (out.u32Width - rect.u32Width) >> 1;
	rect.s32Y = (out.u32Height - rect.u32Height) >> 1;

	return rect;
}

/**************************************************************************
 *   Job related APIs.
 **************************************************************************/
void vpss_gdc_callback(void *pParam, VB_BLK blk)
{
	struct _vpss_gdc_cb_param *_pParam = pParam;

	if (!pParam)
		return;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) usage(%d)\n", _pParam->chn.s32DevId,
		       _pParam->chn.s32ChnId, _pParam->usage);
	mutex_unlock(&mesh[_pParam->chn.s32DevId][_pParam->chn.s32ChnId].lock);
	if (blk != VB_INVALID_HANDLE)
		vb_done_handler(_pParam->chn, CHN_TYPE_OUT,
			&stVpssVbjobs[_pParam->chn.s32DevId].outs[_pParam->chn.s32ChnId], blk);
	vfree(pParam);
}

static u8 get_work_mask(struct cvi_vpss_ctx *ctx)
{
	u8 mask = 0;
	VPSS_CHN VpssChn;

	if (!ctx->isCreated || !ctx->isStarted)
		return 0;

	for (VpssChn = 0; VpssChn < VPSS_MAX_CHN_NUM; ++VpssChn) {
		if (!ctx->stChnCfgs[VpssChn].isEnabled)
			continue;
		mask |= BIT(VpssChn);
	}
	if (mask == 0)
		return 0;

	// img's mask
	mask |= BIT(7);

	return mask;
}

static void _vpss_fill_cvi_buffer(VPSS_CHN VpssChn, struct cvi_buffer *grp_buf,
		uint64_t phy_addr, struct cvi_vpss_ctx *ctx, struct cvi_buffer *buf)
{
	SIZE_S size;
	u8 ldc_wa = CVI_FALSE;

	//workaround for ldc 64-align for width/height.
	if (ctx->stChnCfgs[VpssChn].enRotation != ROTATION_0
		|| (ctx->stChnCfgs[VpssChn].stLDCAttr.bEnable && ctx->stChnCfgs[VpssChn].stLDCAttr.stAttr.bEnHWLDC))
		ldc_wa = CVI_TRUE;

	if (ldc_wa) {
		size.u32Width = ALIGN(ctx->stChnCfgs[VpssChn].stChnAttr.u32Width, LDC_ALIGN);
		size.u32Height = ALIGN(ctx->stChnCfgs[VpssChn].stChnAttr.u32Height, LDC_ALIGN);
	} else {
		size.u32Width = ctx->stChnCfgs[VpssChn].stChnAttr.u32Width;
		size.u32Height = ctx->stChnCfgs[VpssChn].stChnAttr.u32Height;
	}
	base_get_frame_info(ctx->stChnCfgs[VpssChn].stChnAttr.enPixelFormat
			   , size
			   , buf
			   , phy_addr
			   , ctx->stChnCfgs[VpssChn].align);
	buf->s16OffsetTop = 0;
	buf->s16OffsetBottom =
		size.u32Height - ctx->stChnCfgs[VpssChn].stChnAttr.u32Height;
	buf->s16OffsetLeft = 0;
	buf->s16OffsetRight =
		size.u32Width - ctx->stChnCfgs[VpssChn].stChnAttr.u32Width;

	if (grp_buf) {
		buf->u64PTS = grp_buf->u64PTS;
		buf->frm_num = grp_buf->frm_num;
		buf->motion_lv = grp_buf->motion_lv;
		memcpy(buf->motion_table, grp_buf->motion_table, MO_TBL_SIZE);
	}
}

void job_fill_buf(struct cvi_buffer *buf, u64 *addr)
{
	u8 i;

	for (i = 0; i < NUM_OF_PLANES; ++i)
		addr[i] = buf->phy_addr[i];

	if (buf->enPixelFormat == PIXEL_FORMAT_BGR_888_PLANAR) {
		addr[0] = buf->phy_addr[2];
		addr[2] = buf->phy_addr[0];
	}

	if (buf->enCompressMode == COMPRESS_MODE_FRAME)
		addr[3] = buf->compress_expand_addr;
}

static s32 vpss_qbuf(MMF_CHN_S chn, struct cvi_buffer *grp_buf,
	VB_BLK blk, struct cvi_vpss_ctx *ctx, struct cvi_vpss_job *pstJob)
{
	s32 ret;
	struct vb_s *vb = (struct vb_s *)blk;

	_vpss_fill_cvi_buffer(chn.s32ChnId, grp_buf, vb_handle2phys_addr(blk), ctx, &vb->buf);

	ret = vb_qbuf(chn, CHN_TYPE_OUT, &stVpssVbjobs[chn.s32DevId].outs[chn.s32ChnId], blk);
	if (ret != CVI_SUCCESS)
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) qbuf failed\n", chn.s32DevId, chn.s32ChnId);
	else
		job_fill_buf(&vb->buf, pstJob->cfg.astChnCfg[chn.s32ChnId].addr);

	vb_release_block(blk);

	return ret;
}

static s32 vpss_online_qbuf(MMF_CHN_S chn, void *data)
{
	struct cvi_vpss_ctx *ctx;
	struct cvi_vpss_job *pstJob = (struct cvi_vpss_job *)data;
	VB_BLK blk;

	if (check_vpss_grp_valid(chn.s32DevId))
		return CVI_SUCCESS;
	if (!vpssCtx[chn.s32DevId])
		return CVI_SUCCESS;
	if (!vpssCtx[chn.s32DevId]->stChnCfgs[chn.s32ChnId].isEnabled)
		return CVI_SUCCESS;

	ctx = vpssCtx[chn.s32DevId];
	blk = vb_get_block_with_id(ctx->stChnCfgs[chn.s32ChnId].VbPool,
		ctx->stChnCfgs[chn.s32ChnId].blk_size, CVI_ID_VPSS);
	if (blk == VB_INVALID_HANDLE) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) Can't acquire VB BLK for VPSS\n"
			, chn.s32DevId, chn.s32ChnId);
		return CVI_FAILURE;
	}

	CVI_TRACE_VPSS(CVI_DBG_NOTICE, "Grp(%d) Chn(%d) acquire VB BLK\n", chn.s32DevId, chn.s32ChnId);
	return vpss_qbuf(chn, NULL, blk, ctx, pstJob);
}

static s32 fill_buffers(struct cvi_vpss_ctx *ctx, struct cvi_vpss_job *pstJob)
{
	s32 ret = CVI_SUCCESS;
	VB_BLK blk[VPSS_MAX_CHN_NUM] = { [0 ... VPSS_MAX_CHN_NUM - 1] = VB_INVALID_HANDLE };
	VB_BLK blk_grp;
	VPSS_GRP VpssGrp = ctx->VpssGrp;
	u8 online_from_isp = ctx->online_from_isp;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = 0};
	VPSS_CHN VpssChn = 0;
	struct VPSS_CHN_CFG *pstChnCfg;
	struct cvi_buffer *buf;
	struct cvi_buffer *buf_in = NULL;
	VB_POOL poolid = VB_INVALID_POOLID;

	if (!online_from_isp && base_mod_jobs_waitq_empty(&stVpssVbjobs[VpssGrp].ins))
		return CVI_ERR_VPSS_BUF_EMPTY;

	// get buffers.
	for (VpssChn = 0; VpssChn < VPSS_MAX_CHN_NUM; ++VpssChn) {
		pstChnCfg = &ctx->stChnCfgs[VpssChn];
		if (!pstChnCfg->isEnabled)
			continue;
		if (pstChnCfg->isDrop)
			continue;

		chn.s32ChnId = VpssChn;
		pstJob->cfg.astChnCfg[VpssChn].addr[0] = 0;

		// chn buffer from user
		if (!base_mod_jobs_waitq_empty(&stVpssVbjobs[VpssGrp].outs[VpssChn])) {
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) chn buffer from user.\n", VpssGrp, VpssChn);

			buf = base_mod_jobs_enque_work(&stVpssVbjobs[VpssGrp].outs[VpssChn]);
			if (!buf) {
				CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) qbuf failed.\n", VpssGrp, VpssChn);
				ret = CVI_ERR_VPSS_NOTREADY;
				break;
			}

			// Implement cvi_qbuf in user space
			job_fill_buf(buf, pstJob->cfg.astChnCfg[VpssChn].addr);
			pstJob->cfg.astChnCfg[VpssChn].bytesperline[0] = buf->stride[0];
			pstJob->cfg.astChnCfg[VpssChn].bytesperline[1] = buf->stride[1];
			continue;
		}

		// chn buffer from pool
		blk[VpssChn] = vb_get_block_with_id(ctx->stChnCfgs[VpssChn].VbPool,
						ctx->stChnCfgs[VpssChn].blk_size, CVI_ID_VPSS);
		if (blk[VpssChn] == VB_INVALID_HANDLE) {
			if (online_from_isp) {
				if (ctx->stChnCfgs[VpssChn].VbPool == VB_INVALID_POOLID)
					poolid = find_vb_pool(ctx->stChnCfgs[VpssChn].blk_size);
				else
					poolid = ctx->stChnCfgs[VpssChn].VbPool;
				vb_acquire_block(vpss_online_qbuf, chn, poolid, pstJob);
				CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) Chn(%d) acquire VB BLK later\n"
					, VpssGrp, VpssChn);
			} else {
				CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) Can't acquire VB BLK for VPSS\n"
					, VpssGrp, VpssChn);
				ret = CVI_ERR_VPSS_NOBUF;
				break;
			}
		}
	}
	if (ret != CVI_SUCCESS)
		goto ERR_FILL_BUF;

	if (!online_from_isp) {
		buf_in = base_mod_jobs_enque_work(&stVpssVbjobs[VpssGrp].ins);
		if (buf_in == NULL) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) qbuf failed.\n", VpssGrp);
			ret = CVI_ERR_VPSS_NOTREADY;
			goto ERR_FILL_BUF;
		}

		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "grp(%d) buf: 0x%lx-0x%lx-0x%lx\n", VpssGrp,
			(unsigned long)buf_in->phy_addr[0], (unsigned long)buf_in->phy_addr[1],
			(unsigned long)buf_in->phy_addr[2]);

		job_fill_buf(buf_in, pstJob->cfg.stGrpCfg.addr);
	}

	for (VpssChn = 0; VpssChn < VPSS_MAX_CHN_NUM; ++VpssChn) {
		if (blk[VpssChn] == VB_INVALID_HANDLE)
			continue;
		chn.s32ChnId = VpssChn;

		vpss_qbuf(chn, buf_in, blk[VpssChn], ctx, pstJob);
	}

	return ret;
ERR_FILL_BUF:
	while ((VpssChn > 0) && (--VpssChn < VPSS_MAX_CHN_NUM)) {
		if (blk[VpssChn] != VB_INVALID_HANDLE)
			vb_release_block(blk[VpssChn]);
	}
	blk_grp = base_mod_jobs_waitq_pop(&stVpssVbjobs[VpssGrp].ins);
	if (blk_grp != VB_INVALID_HANDLE)
		vb_release_block(blk_grp);

	return ret;
}

static void release_buffers(struct cvi_vpss_ctx *ctx)
{
	VPSS_GRP VpssGrp = ctx->VpssGrp;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = 0};
	VB_BLK blk;
	VPSS_CHN VpssChn;
	VB_POOL poolid = VB_INVALID_POOLID;

	if (!ctx->online_from_isp) {
		chn.s32ChnId = 0;
		if (!base_mod_jobs_workq_empty(&stVpssVbjobs[VpssGrp].ins)) {
			vb_dqbuf(chn, &stVpssVbjobs[VpssGrp].ins, &blk);
			if (blk != VB_INVALID_HANDLE)
				vb_release_block(blk);
		}
	}

	for (VpssChn = 0; VpssChn < VPSS_MAX_CHN_NUM; ++VpssChn) {
		struct VPSS_CHN_CFG *pstChnCfg = &ctx->stChnCfgs[VpssChn];

		if (!pstChnCfg->isEnabled)
			continue;

		chn.s32ChnId = VpssChn;

		if (ctx->stChnCfgs[VpssChn].VbPool == VB_INVALID_POOLID)
			poolid = find_vb_pool(ctx->stChnCfgs[VpssChn].blk_size);
		else
			poolid = ctx->stChnCfgs[VpssChn].VbPool;
		vb_cancel_block(chn, poolid);

		while (!base_mod_jobs_workq_empty(&stVpssVbjobs[VpssGrp].outs[VpssChn])) {
			vb_dqbuf(chn, &stVpssVbjobs[VpssGrp].outs[VpssChn], &blk);
			if (blk != VB_INVALID_HANDLE)
				vb_release_block(blk);
		}
	}
}

static void _release_vpss_waitq(MMF_CHN_S chn, enum CHN_TYPE_E chn_type)
{
	VB_BLK blk_grp;

	if (chn_type == CHN_TYPE_OUT)
		blk_grp = base_mod_jobs_waitq_pop(&stVpssVbjobs[chn.s32DevId].outs[chn.s32ChnId]);
	else
		blk_grp = base_mod_jobs_waitq_pop(&stVpssVbjobs[chn.s32DevId].ins);

	if (blk_grp != VB_INVALID_HANDLE)
		vb_release_block(blk_grp);
}

static void _vpss_online_set_mlv_info(struct vb_s *vb)
{
	u8 snr_num = vb->buf.dev_num;

	vb->buf.motion_lv = g_mlv_i[snr_num].mlv_i_level;
	memcpy(vb->buf.motion_table, g_mlv_i[snr_num].mlv_i_table, sizeof(vb->buf.motion_table));
}

static s32 _vpss_online_get_dpcm_wr_crop(u8 snr_num,
	RECT_S *dpcm_wr_crop, SIZE_S src_size)
{
#if 0
	struct crop_size crop = g_dpcm_wr_i.dpcm_wr_i_crop[snr_num];
	s32 bRet = CVI_SUCCESS;

	if (g_dpcm_wr_i.dpcm_wr_i_dpcmon) {
		// check if dpcm_wr crop valid
		if (crop.end_x <= crop.start_x ||
			crop.end_y <= crop.start_y ||
			crop.end_x > src_size.u32Width ||
			crop.end_y > src_size.u32Height ||
			((u32)(crop.end_x - crop.start_x) == src_size.u32Width &&
			(u32)(crop.end_y - crop.start_y) == src_size.u32Height))
			bRet = CVI_ERR_VPSS_ILLEGAL_PARAM;
		else {
			dpcm_wr_crop->s32X = (s32)crop.start_x;
			dpcm_wr_crop->s32Y = (s32)crop.start_y;
			dpcm_wr_crop->u32Width = (u32)(crop.end_x - crop.start_x);
			dpcm_wr_crop->u32Height = (u32)(crop.end_y - crop.start_y);
		}
	} else
		bRet = CVI_ERR_VPSS_NOT_PERM;

	return bRet;
#else
	if (snr_num >= VPSS_MAX_GRP_NUM || !dpcm_wr_crop || !src_size.u32Width)
		return CVI_FAILURE;

	return CVI_ERR_VPSS_NOT_PERM;
#endif
}

/*
 * _vpss_get_union_crop() - get union crop area of cropA & cropB.
 * If two crop area has no union, return cropA
 */
static RECT_S _vpss_get_union_crop(RECT_S cropA, RECT_S cropB)
{
	RECT_S union_crop;

	// check if no union
	if ((cropA.s32X >= cropB.s32X + (s32)cropB.u32Width) ||
		(cropA.s32Y >= cropB.s32Y + (s32)cropB.u32Height) ||
		(cropB.s32X >= cropA.s32X + (s32)cropA.u32Width) ||
		(cropB.s32Y >= cropA.s32Y + (s32)cropA.u32Height))
		return cropA;

	union_crop.s32X = (cropA.s32X > cropB.s32X) ? cropA.s32X : cropB.s32X;
	union_crop.s32Y = (cropA.s32Y > cropB.s32Y) ? cropA.s32Y : cropB.s32Y;
	union_crop.u32Width =
		((cropA.s32X + (s32)cropA.u32Width) < (cropB.s32X + (s32)cropB.u32Width)) ?
		(u32)(cropA.s32X + (s32)cropA.u32Width - union_crop.s32X) :
		(u32)(cropB.s32X + (s32)cropB.u32Width - union_crop.s32X);
	union_crop.u32Height =
		((cropA.s32Y + (s32)cropA.u32Height) < (cropB.s32Y + (s32)cropB.u32Height)) ?
		(u32)(cropA.s32Y + (s32)cropA.u32Height - union_crop.s32Y) :
		(u32)(cropB.s32Y + (s32)cropB.u32Height - union_crop.s32Y);

	return union_crop;
}

/* _is_frame_crop_changed() - to see if frame's crop info changed
 */
static u8 _is_frame_crop_changed(VPSS_GRP VpssGrp, struct cvi_vpss_ctx *ctx)
{
	struct vb_s *vb_in = NULL;
	u8 ret = CVI_FALSE;
	struct vb_jobs_t *jobs;

	if (base_mod_jobs_waitq_empty(&stVpssVbjobs[VpssGrp].ins))
		return CVI_FALSE;

	jobs = &stVpssVbjobs[VpssGrp].ins;
	FIFO_GET_FRONT(&jobs->waitq, &vb_in);

	if (vb_in == NULL) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) unexpected empty waitq\n", VpssGrp);
		return CVI_FALSE;
	}

	//GDC 64 align case
	if ((ctx->s16OffsetLeft != vb_in->buf.s16OffsetLeft) ||
		(ctx->s16OffsetTop != vb_in->buf.s16OffsetTop) ||
		(ctx->s16OffsetRight != vb_in->buf.s16OffsetRight) ||
		(ctx->s16OffsetBottom != vb_in->buf.s16OffsetBottom)) {
		ctx->s16OffsetLeft = vb_in->buf.s16OffsetLeft;
		ctx->s16OffsetTop = vb_in->buf.s16OffsetTop;
		ctx->s16OffsetRight = vb_in->buf.s16OffsetRight;
		ctx->s16OffsetBottom = vb_in->buf.s16OffsetBottom;
		ret = CVI_TRUE;
	}

	//dis case
	if (memcmp(&vb_in->buf.frame_crop, &ctx->frame_crop, sizeof(vb_in->buf.frame_crop))) {
		u8 chk_width_even = IS_FMT_YUV420(ctx->stGrpAttr.enPixelFormat) ||
				      IS_FMT_YUV422(ctx->stGrpAttr.enPixelFormat);
		u8 chk_height_even = IS_FMT_YUV420(ctx->stGrpAttr.enPixelFormat);

		if (chk_width_even && ((vb_in->buf.frame_crop.end_x - vb_in->buf.frame_crop.start_x) & 0x01)) {
			CVI_TRACE_VPSS(CVI_DBG_WARN, "VpssGrp(%d) frame-crop invalid - start_x(%d) end_x(%d)\n",
				       VpssGrp, vb_in->buf.frame_crop.start_x, vb_in->buf.frame_crop.end_x);
			CVI_TRACE_VPSS(CVI_DBG_WARN, "frame-crop's width should be even for yuv format\n");
			return ret;
		}
		if (chk_height_even && ((vb_in->buf.frame_crop.end_y - vb_in->buf.frame_crop.start_y) & 0x01)) {
			CVI_TRACE_VPSS(CVI_DBG_WARN, "VpssGrp(%d) frame-crop invalid - start_y(%d) end_y(%d)\n",
				       VpssGrp, vb_in->buf.frame_crop.start_y, vb_in->buf.frame_crop.end_y);
			CVI_TRACE_VPSS(CVI_DBG_WARN, "frame-crop's height should be even for yuv format\n");
			return ret;
		}

		ctx->frame_crop = vb_in->buf.frame_crop;
		ret = CVI_TRUE;
	}
	return ret;
}

/* _is_frame_crop_valid() - to see if frame's crop info valid/enabled
 */
static bool _is_frame_crop_valid(struct cvi_vpss_ctx *ctx)
{
	return (ctx->frame_crop.end_x > ctx->frame_crop.start_x &&
		ctx->frame_crop.end_y > ctx->frame_crop.start_y &&
		ctx->frame_crop.end_x <= ctx->stGrpAttr.u32MaxW &&
		ctx->frame_crop.end_y <= ctx->stGrpAttr.u32MaxH &&
		!((u32)(ctx->frame_crop.end_x - ctx->frame_crop.start_x)
			== ctx->stGrpAttr.u32MaxW &&
		  (u32)(ctx->frame_crop.end_y - ctx->frame_crop.start_y)
			== ctx->stGrpAttr.u32MaxH));
}

static void _vpss_over_crop_resize
	(struct cvi_vpss_grp_cfg *pstGrpHwCfg, RECT_S crop_rect, RECT_S *resize_rect)
{
	u32 scale = crop_rect.u32Width * crop_rect.u32Height;
	u32 ratio;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "rect_crop (l=%d, t=%d, w=%d, h=%d)\n",
			pstGrpHwCfg->crop.left,
			pstGrpHwCfg->crop.top,
			pstGrpHwCfg->crop.width,
			pstGrpHwCfg->crop.height);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "rect before resize(%d, %d, %d, %d)\n"
				, resize_rect->s32X, resize_rect->s32Y
				, resize_rect->u32Width, resize_rect->u32Height);

	if (crop_rect.s32X < 0) {
		//ratio = (float)ABS(crop_rect.s32X) / crop_rect.u32Width;
		//resize_rect->s32X += (s32)(resize_rect->u32Width * ratio + 0.5);
		//resize_rect->u32Width -= (u32)(resize_rect->u32Width * ratio + 0.5);
		ratio = ABS(crop_rect.s32X) * crop_rect.u32Height;
		resize_rect->s32X += (s32)(resize_rect->u32Width * ratio + scale / 2) / scale;
		resize_rect->u32Width -= (u32)(resize_rect->u32Width * ratio + scale / 2) / scale;
	}

	if (crop_rect.s32X + crop_rect.u32Width > pstGrpHwCfg->crop.width) {
		//ratio = (float)(crop_rect.s32X + crop_rect.u32Width - pstGrpHwCfg->rect_crop.width)
		//	/ (crop_rect.u32Width);
		//resize_rect->u32Width -= (u32)(resize_rect->u32Width * ratio + 0.5);
		ratio = (crop_rect.s32X + crop_rect.u32Width - pstGrpHwCfg->crop.width)
			* (crop_rect.u32Height);
		resize_rect->u32Width -= (u32)(resize_rect->u32Width * ratio + scale / 2) / scale;
	}

	if (crop_rect.s32Y < 0) {
		//ratio = (float)ABS(crop_rect.s32Y) / crop_rect.u32Height;
		//resize_rect->s32Y += (s32)(resize_rect->u32Height * ratio + 0.5);
		//resize_rect->u32Height -= (u32)(resize_rect->u32Height * ratio + 0.5);
		ratio = ABS(crop_rect.s32Y) * crop_rect.u32Width;
		resize_rect->s32Y += (s32)(resize_rect->u32Height * ratio + scale / 2) / scale;
		resize_rect->u32Height -= (u32)(resize_rect->u32Height * ratio + scale / 2) / scale;
	}

	if (crop_rect.s32Y + crop_rect.u32Height > pstGrpHwCfg->crop.height) {
		//ratio = (float)(crop_rect.s32Y + crop_rect.u32Height - pstGrpHwCfg->rect_crop.height)
		//	/ (crop_rect.u32Height);
		//resize_rect->u32Height -= (u32)(resize_rect->u32Height * ratio + 0.5);
		ratio = (crop_rect.s32Y + crop_rect.u32Height - pstGrpHwCfg->crop.height)
			* (crop_rect.u32Width);
		resize_rect->u32Height -= (u32)(resize_rect->u32Height * ratio + scale / 2) / scale;
	}

	CVI_TRACE_VPSS(CVI_DBG_INFO, "rect after resize(%d, %d, %d, %d)\n"
			, resize_rect->s32X, resize_rect->s32Y
			, resize_rect->u32Width, resize_rect->u32Height);
}

/*
 * @param VpssChn: VPSS Chn to update cfg
 * @param ctx: VPSS ctx which records settings of this grp
 */
void _vpss_chn_hw_cfg_update(VPSS_CHN VpssChn, struct cvi_vpss_ctx *ctx)
{
	u8 i;
	VPSS_GRP VpssGrp = ctx->VpssGrp;
	u8 online_from_isp = ctx->online_from_isp;
	struct cvi_vpss_grp_cfg *pstGrpHwCfg = &ctx->hw_cfg.stGrpCfg;
	struct VPSS_CHN_CFG *pstChnCfg = &ctx->stChnCfgs[VpssChn];
	struct cvi_vpss_chn_cfg *pstHwChnCfg = &ctx->hw_cfg.astChnCfg[VpssChn];
	VB_CAL_CONFIG_S stVbCalConfig;
	RECT_S chnCrop = pstChnCfg->stCropInfo.stCropRect;
	u8 bCropOverSrcRange = CVI_FALSE;

	COMMON_GetPicBufferConfig(pstChnCfg->stChnAttr.u32Width, pstChnCfg->stChnAttr.u32Height,
		pstChnCfg->stChnAttr.enPixelFormat, DATA_BITWIDTH_8
		, COMPRESS_MODE_NONE, pstChnCfg->align, &stVbCalConfig);
	pstHwChnCfg->bytesperline[0] = stVbCalConfig.u32MainStride;
	pstHwChnCfg->bytesperline[1] = stVbCalConfig.u32CStride;

	if (pstChnCfg->stCropInfo.bEnable) {
		//CVI_FLOAT h_ratio = 1.0f, v_ratio = 1.0f;
		u32 scale = ctx->stGrpAttr.u32MaxW * ctx->stGrpAttr.u32MaxH;
		u32 h_ratio = scale, v_ratio = scale;
		u64 left, right, height, width;

		if (!online_from_isp) {
			// use ratio-coordinate if dis enabled.
			if (_is_frame_crop_valid(ctx)) {
				//h_ratio = (CVI_FLOAT)pstGrpHwCfg->rect_crop.width / ctx->stGrpAttr.u32MaxW;
				//v_ratio = (CVI_FLOAT)pstGrpHwCfg->rect_crop.height / ctx->stGrpAttr.u32MaxH;
				h_ratio = pstGrpHwCfg->crop.width * ctx->stGrpAttr.u32MaxH;
				v_ratio = pstGrpHwCfg->crop.height * ctx->stGrpAttr.u32MaxW;
			}
		} else {
			RECT_S dpcm_wr_crop;
			SIZE_S chn_src_size;
			s32 bRet;

			chn_src_size.u32Width = pstGrpHwCfg->crop.width;
			chn_src_size.u32Height = pstGrpHwCfg->crop.height;
			bRet = _vpss_online_get_dpcm_wr_crop(VpssGrp, &dpcm_wr_crop, chn_src_size);
			if (bRet == CVI_SUCCESS)
				chnCrop = _vpss_get_union_crop(dpcm_wr_crop, chnCrop);
		}
		//pstHwChnCfg->rect_crop.left = chnCrop.s32X = chnCrop.s32X * h_ratio;
		//pstHwChnCfg->rect_crop.top = chnCrop.s32Y = chnCrop.s32Y * v_ratio;
		//pstHwChnCfg->rect_crop.width = chnCrop.u32Width = chnCrop.u32Width * h_ratio;
		//pstHwChnCfg->rect_crop.height = chnCrop.u32Height = chnCrop.u32Height * v_ratio;
		left = chnCrop.s32X * (s64)h_ratio;
		do_div(left, scale);
		pstHwChnCfg->crop.left = chnCrop.s32X = left;

		right = chnCrop.s32Y * (s64)v_ratio;
		do_div(right, scale);
		pstHwChnCfg->crop.top = chnCrop.s32Y = right;

		width = chnCrop.u32Width * (s64)h_ratio;
		do_div(width, scale);
		pstHwChnCfg->crop.width = chnCrop.u32Width = (u32)width;

		height = chnCrop.u32Height * (s64)v_ratio;
		do_div(height, scale);
		pstHwChnCfg->crop.height = chnCrop.u32Height = (u32)height;

		// check if crop rect contains the region outside input src
		if (chnCrop.s32X < 0) {
			pstHwChnCfg->crop.left = 0;
			pstHwChnCfg->crop.width = (chnCrop.u32Width - ABS(chnCrop.s32X));
			bCropOverSrcRange = CVI_TRUE;
		}
		if (chnCrop.s32X + chnCrop.u32Width > pstGrpHwCfg->crop.width) {
			pstHwChnCfg->crop.width = pstGrpHwCfg->crop.width - pstHwChnCfg->crop.left;
			bCropOverSrcRange = CVI_TRUE;
		}

		if (chnCrop.s32Y < 0) {
			pstHwChnCfg->crop.top = 0;
			pstHwChnCfg->crop.height = (chnCrop.u32Height - ABS(chnCrop.s32Y));
			bCropOverSrcRange = CVI_TRUE;
		}
		if (chnCrop.s32Y + chnCrop.u32Height > pstGrpHwCfg->crop.height) {
			pstHwChnCfg->crop.height = pstGrpHwCfg->crop.height - pstHwChnCfg->crop.top;
			bCropOverSrcRange = CVI_TRUE;
		}
	} else {
		pstHwChnCfg->crop.left = pstHwChnCfg->crop.top
			= chnCrop.s32X = chnCrop.s32Y = 0;
		pstHwChnCfg->crop.width = chnCrop.u32Width = pstGrpHwCfg->crop.width;
		pstHwChnCfg->crop.height = chnCrop.u32Height = pstGrpHwCfg->crop.height;
		if (online_from_isp) {
			RECT_S dpcm_wr_crop;
			SIZE_S chn_src_size;
			s32 bRet;

			chn_src_size.u32Width = pstGrpHwCfg->crop.width;
			chn_src_size.u32Height = pstGrpHwCfg->crop.height;
			bRet = _vpss_online_get_dpcm_wr_crop(VpssGrp, &dpcm_wr_crop, chn_src_size);
			if (bRet == CVI_SUCCESS) {
				pstHwChnCfg->crop.left = chnCrop.s32X = dpcm_wr_crop.s32X;
				pstHwChnCfg->crop.top = chnCrop.s32Y = dpcm_wr_crop.s32Y;
				pstHwChnCfg->crop.width = chnCrop.u32Width = dpcm_wr_crop.u32Width;
				pstHwChnCfg->crop.height = chnCrop.u32Height = dpcm_wr_crop.u32Height;
			}
		}
	}
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "grp(%d) chn(%d) rect(%d %d %d %d)\n", VpssGrp, VpssChn
			, pstHwChnCfg->crop.left, pstHwChnCfg->crop.top
			, pstHwChnCfg->crop.width, pstHwChnCfg->crop.height);

	if (pstChnCfg->stChnAttr.stAspectRatio.enMode == ASPECT_RATIO_AUTO) {
		SIZE_S in, out;
		RECT_S rect;
		u8 is_border_enabled = CVI_FALSE;

		in.u32Width = chnCrop.u32Width;
		in.u32Height = chnCrop.u32Height;
		out.u32Width = pstChnCfg->stChnAttr.u32Width;
		out.u32Height = pstChnCfg->stChnAttr.u32Height;
		rect = aspect_ratio_resize(in, out);

		if (bCropOverSrcRange)
			_vpss_over_crop_resize(pstGrpHwCfg, chnCrop, &rect);

		is_border_enabled = pstChnCfg->stChnAttr.stAspectRatio.bEnableBgColor
			&& ((rect.u32Width != pstChnCfg->stChnAttr.u32Width)
			 || (rect.u32Height != pstChnCfg->stChnAttr.u32Height));

		CVI_TRACE_VPSS(CVI_DBG_INFO, "input(%d %d) output(%d %d)\n"
				, in.u32Width, in.u32Height, out.u32Width, out.u32Height);
		CVI_TRACE_VPSS(CVI_DBG_INFO, "ratio (%d %d %d %d) border_enabled(%d)\n"
				, rect.s32X, rect.s32Y, rect.u32Width, rect.u32Height, is_border_enabled);

		pstHwChnCfg->border_cfg.enable = is_border_enabled;
		pstHwChnCfg->border_cfg.offset_x = rect.s32X;
		pstHwChnCfg->border_cfg.offset_y = rect.s32Y;
		pstHwChnCfg->border_cfg.bg_color[2] = pstChnCfg->stChnAttr.stAspectRatio.u32BgColor & 0xff;
		pstHwChnCfg->border_cfg.bg_color[1] = (pstChnCfg->stChnAttr.stAspectRatio.u32BgColor >> 8) & 0xff;
		pstHwChnCfg->border_cfg.bg_color[0] = (pstChnCfg->stChnAttr.stAspectRatio.u32BgColor >> 16) & 0xff;

		if (is_border_enabled) {
			pstHwChnCfg->dst_rect.left = pstHwChnCfg->dst_rect.top = 0;
		} else {
			pstHwChnCfg->dst_rect.left = rect.s32X;
			pstHwChnCfg->dst_rect.top = rect.s32Y;
		}


		if (IS_FMT_YUV420(pstChnCfg->stChnAttr.enPixelFormat)
			|| IS_FMT_YUV422(pstChnCfg->stChnAttr.enPixelFormat)) {
			pstHwChnCfg->dst_rect.width = rect.u32Width & ~0x01;
			pstHwChnCfg->dst_rect.left &= ~0x01;
		} else
			pstHwChnCfg->dst_rect.width = rect.u32Width;

		if (IS_FMT_YUV420(pstChnCfg->stChnAttr.enPixelFormat))
			pstHwChnCfg->dst_rect.height = rect.u32Height & ~0x01;
		else
			pstHwChnCfg->dst_rect.height = rect.u32Height;
	} else if (pstChnCfg->stChnAttr.stAspectRatio.enMode == ASPECT_RATIO_MANUAL) {
		RECT_S rect = pstChnCfg->stChnAttr.stAspectRatio.stVideoRect;
		u8 is_border_enabled = CVI_FALSE;

		if (bCropOverSrcRange)
			_vpss_over_crop_resize(pstGrpHwCfg, chnCrop, &rect);

		is_border_enabled = pstChnCfg->stChnAttr.stAspectRatio.bEnableBgColor
			&& ((rect.u32Width != pstChnCfg->stChnAttr.u32Width)
			 || (rect.u32Height != pstChnCfg->stChnAttr.u32Height));

		CVI_TRACE_VPSS(CVI_DBG_INFO, "rect(%d %d %d %d) border_enabled(%d)\n"
				, rect.s32X, rect.s32Y, rect.u32Width, rect.u32Height, is_border_enabled);

		if (is_border_enabled) {
			pstHwChnCfg->dst_rect.left = pstHwChnCfg->dst_rect.top = 0;
		} else {
			pstHwChnCfg->dst_rect.left = rect.s32X;
			pstHwChnCfg->dst_rect.top = rect.s32Y;
		}
		pstHwChnCfg->dst_rect.width = rect.u32Width;
		pstHwChnCfg->dst_rect.height = rect.u32Height;

		pstHwChnCfg->border_cfg.enable = is_border_enabled;
		pstHwChnCfg->border_cfg.offset_x = rect.s32X;
		pstHwChnCfg->border_cfg.offset_y = rect.s32Y;
		pstHwChnCfg->border_cfg.bg_color[2] = pstChnCfg->stChnAttr.stAspectRatio.u32BgColor & 0xff;
		pstHwChnCfg->border_cfg.bg_color[1] = (pstChnCfg->stChnAttr.stAspectRatio.u32BgColor >> 8) & 0xff;
		pstHwChnCfg->border_cfg.bg_color[0] = (pstChnCfg->stChnAttr.stAspectRatio.u32BgColor >> 16) & 0xff;
	} else {
		RECT_S rect;

		rect.s32X = rect.s32Y = 0;
		rect.u32Width = pstChnCfg->stChnAttr.u32Width;
		rect.u32Height = pstChnCfg->stChnAttr.u32Height;
		if (bCropOverSrcRange)
			_vpss_over_crop_resize(pstGrpHwCfg, chnCrop, &rect);

		pstHwChnCfg->dst_rect.left = pstHwChnCfg->dst_rect.top = 0;
		pstHwChnCfg->dst_rect.width = rect.u32Width;
		pstHwChnCfg->dst_rect.height = rect.u32Height;
		if (bCropOverSrcRange) {
			pstHwChnCfg->border_cfg.enable = CVI_TRUE;
			pstHwChnCfg->border_cfg.offset_x = rect.s32X;
			pstHwChnCfg->border_cfg.offset_y = rect.s32Y;
			pstHwChnCfg->border_cfg.bg_color[2] = 0;
			pstHwChnCfg->border_cfg.bg_color[1] = 0;
			pstHwChnCfg->border_cfg.bg_color[0] = 0;
		} else {
			pstHwChnCfg->border_cfg.enable = CVI_FALSE;
		}
	}

	if (pstHwChnCfg->dst_rect.width * VPSS_MAX_ZOOMOUT  < pstHwChnCfg->crop.width
		|| pstHwChnCfg->dst_rect.height * VPSS_MAX_ZOOMOUT  < pstHwChnCfg->crop.height) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "zoom out over %d times, sc in(w:%d, h:%d), sc out(w:%d, h:%d)\n"
			, VPSS_MAX_ZOOMOUT, pstHwChnCfg->crop.width, pstHwChnCfg->crop.height
			, pstHwChnCfg->dst_rect.width, pstHwChnCfg->dst_rect.height);

		pstHwChnCfg->crop.width = pstHwChnCfg->dst_rect.width * VPSS_MAX_ZOOMOUT;
		pstHwChnCfg->crop.height = pstHwChnCfg->dst_rect.height * VPSS_MAX_ZOOMOUT;
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Modify to sc in(w:%d, h:%d), sc out(w:%d, h:%d)\n"
			, pstHwChnCfg->crop.width, pstHwChnCfg->crop.height
			, pstHwChnCfg->dst_rect.width, pstHwChnCfg->dst_rect.height);
	}

	pstHwChnCfg->quant_cfg.enable = pstChnCfg->stChnAttr.stNormalize.bEnable;
	if (pstChnCfg->stChnAttr.stNormalize.bEnable) {
		struct vpss_int_normalize *int_norm =
			(struct vpss_int_normalize *)&ctx->stChnCfgs[VpssChn].stChnAttr.stNormalize;

		for (i = 0; i < 3; i++) {
			pstHwChnCfg->quant_cfg.sc_frac[i] = int_norm->sc_frac[i];
			pstHwChnCfg->quant_cfg.sub[i] = int_norm->sub[i];
			pstHwChnCfg->quant_cfg.sub_frac[i] = int_norm->sub_frac[i];
		}

		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sc_frac(0x%x, 0x%x, 0x%x)\n",
			pstHwChnCfg->quant_cfg.sc_frac[0],
			pstHwChnCfg->quant_cfg.sc_frac[1],
			pstHwChnCfg->quant_cfg.sc_frac[2]);

		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sub(0x%x, 0x%x, 0x%x), sub_frac(0x%x, 0x%x, 0x%x)\n",
			pstHwChnCfg->quant_cfg.sub[0],
			pstHwChnCfg->quant_cfg.sub[1],
			pstHwChnCfg->quant_cfg.sub[2],
			pstHwChnCfg->quant_cfg.sub_frac[0],
			pstHwChnCfg->quant_cfg.sub_frac[1],
			pstHwChnCfg->quant_cfg.sub_frac[2]);

		pstHwChnCfg->quant_cfg.rounding = (enum cvi_sc_quant_rounding)int_norm->rounding;
	}else {
		pstHwChnCfg->YRatio = pstChnCfg->YRatio;
	}

	switch (pstChnCfg->enCoef) {
	default:
	case VPSS_SCALE_COEF_BICUBIC:
		pstHwChnCfg->sc_coef = CVI_SC_SCALING_COEF_BICUBIC;
		break;
	case VPSS_SCALE_COEF_BILINEAR:
		pstHwChnCfg->sc_coef = CVI_SC_SCALING_COEF_BILINEAR;
		break;
	case VPSS_SCALE_COEF_NEAREST:
		pstHwChnCfg->sc_coef = CVI_SC_SCALING_COEF_NEAREST;
		break;
	case VPSS_SCALE_COEF_BICUBIC_OPENCV:
		pstHwChnCfg->sc_coef = CVI_SC_SCALING_COEF_BICUBIC_OPENCV;
		break;
	}

	for (i = 0; i < VPSS_RECT_NUM; i++) {
		RECT_S rect;
		u16 u16Thick;

		pstHwChnCfg->border_vpp_cfg[i].enable = pstChnCfg->stDrawRect.astRect[i].bEnable;
		if (pstHwChnCfg->border_vpp_cfg[i].enable) {
			pstHwChnCfg->border_vpp_cfg[i].bg_color[0] = (pstChnCfg->stDrawRect.astRect[i].u32BgColor >> 16) & 0xff;
			pstHwChnCfg->border_vpp_cfg[i].bg_color[1] = (pstChnCfg->stDrawRect.astRect[i].u32BgColor >> 8) & 0xff;
			pstHwChnCfg->border_vpp_cfg[i].bg_color[2] = pstChnCfg->stDrawRect.astRect[i].u32BgColor & 0xff;

			rect = pstChnCfg->stDrawRect.astRect[i].stRect;
			u16Thick = pstChnCfg->stDrawRect.astRect[i].u16Thick;
			if ((rect.s32X + rect.u32Width) > pstChnCfg->stChnAttr.u32Width)
				rect.u32Width = pstChnCfg->stChnAttr.u32Width - rect.s32X;
			if ((rect.s32Y + rect.u32Height) > pstChnCfg->stChnAttr.u32Height)
				rect.u32Height = pstChnCfg->stChnAttr.u32Height - rect.s32Y;

			pstHwChnCfg->border_vpp_cfg[i].outside.start_x = rect.s32X;
			pstHwChnCfg->border_vpp_cfg[i].outside.start_y = rect.s32Y;
			pstHwChnCfg->border_vpp_cfg[i].outside.end_x = rect.s32X + rect.u32Width;
			pstHwChnCfg->border_vpp_cfg[i].outside.end_y = rect.s32Y + rect.u32Height;
			pstHwChnCfg->border_vpp_cfg[i].inside.start_x = rect.s32X + u16Thick;
			pstHwChnCfg->border_vpp_cfg[i].inside.start_y = rect.s32Y + u16Thick;
			pstHwChnCfg->border_vpp_cfg[i].inside.end_x =
				pstHwChnCfg->border_vpp_cfg[i].outside.end_x - u16Thick;
			pstHwChnCfg->border_vpp_cfg[i].inside.end_y =
				pstHwChnCfg->border_vpp_cfg[i].outside.end_y - u16Thick;
		}
	}

	pstHwChnCfg->convert_to_cfg.enable = pstChnCfg->stConvert.bEnable;
	if (pstHwChnCfg->convert_to_cfg.enable) {
		pstHwChnCfg->convert_to_cfg.a_frac[0] = pstChnCfg->stConvert.u32aFactor[0];
		pstHwChnCfg->convert_to_cfg.a_frac[1] = pstChnCfg->stConvert.u32aFactor[1];
		pstHwChnCfg->convert_to_cfg.a_frac[2] = pstChnCfg->stConvert.u32aFactor[2];
		pstHwChnCfg->convert_to_cfg.b_frac[0] = pstChnCfg->stConvert.u32bFactor[0];
		pstHwChnCfg->convert_to_cfg.b_frac[1] = pstChnCfg->stConvert.u32bFactor[1];
		pstHwChnCfg->convert_to_cfg.b_frac[2] = pstChnCfg->stConvert.u32bFactor[2];
	}

	memcpy(&pstHwChnCfg->csc_cfg, &vpssExtCtx[VpssGrp].chn_csc_cfg[VpssChn], sizeof(pstHwChnCfg->csc_cfg));

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "pstHwChnCfg coef[0][0]: %#4x coef[0][1]: %#4x coef[0][2]: %#4x\n"
		, pstHwChnCfg->csc_cfg.coef[0][0]
		, pstHwChnCfg->csc_cfg.coef[0][1]
		, pstHwChnCfg->csc_cfg.coef[0][2]);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "coef[1][0]: %#4x coef[1][1]: %#4x coef[1][2]: %#4x\n"
		, pstHwChnCfg->csc_cfg.coef[1][0]
		, pstHwChnCfg->csc_cfg.coef[1][1]
		, pstHwChnCfg->csc_cfg.coef[1][2]);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "coef[2][0]: %#4x coef[2][1]: %#4x coef[2][2]: %#4x\n"
		, pstHwChnCfg->csc_cfg.coef[2][0]
		, pstHwChnCfg->csc_cfg.coef[2][1]
		, pstHwChnCfg->csc_cfg.coef[2][2]);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sub[0]: %3d sub[1]: %3d sub[2]: %3d\n"
		, pstHwChnCfg->csc_cfg.sub[0]
		, pstHwChnCfg->csc_cfg.sub[1]
		, pstHwChnCfg->csc_cfg.sub[2]);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "add[0]: %3d add[1]: %3d add[2]: %3d\n"
		, pstHwChnCfg->csc_cfg.add[0]
		, pstHwChnCfg->csc_cfg.add[1]
		, pstHwChnCfg->csc_cfg.add[2]);
}

/*
 * @param ctx: VPSS ctx which records settings of this grp
 * @param pstGrpHwCfg: cfg to be updated
 */
void _vpss_grp_hw_cfg_update(struct cvi_vpss_ctx *ctx)
{
	VPSS_GRP VpssGrp = ctx->VpssGrp;
	VB_CAL_CONFIG_S stVbCalConfig;
	//struct sclr_csc_matrix *mtrx;
	struct cvi_vpss_grp_cfg *pstGrpHwCfg = &ctx->hw_cfg.stGrpCfg;

	COMMON_GetPicBufferConfig(ctx->stGrpAttr.u32MaxW, ctx->stGrpAttr.u32MaxH,
		ctx->stGrpAttr.enPixelFormat, DATA_BITWIDTH_8
		, COMPRESS_MODE_NONE, DEFAULT_ALIGN, &stVbCalConfig);
	pstGrpHwCfg->bytesperline[0] = stVbCalConfig.u32MainStride;
	pstGrpHwCfg->bytesperline[1] = stVbCalConfig.u32CStride;

	// frame_crop applied if valid
	if (_is_frame_crop_valid(ctx)) {
		// for frame crop.
		RECT_S grp_crop;

		grp_crop.s32X = ctx->frame_crop.start_x;
		grp_crop.s32Y = ctx->frame_crop.start_y;
		grp_crop.u32Width = ctx->frame_crop.end_x - ctx->frame_crop.start_x;
		grp_crop.u32Height = ctx->frame_crop.end_y - ctx->frame_crop.start_y;
		if (ctx->stGrpCropInfo.bEnable)
			grp_crop = _vpss_get_union_crop(grp_crop, ctx->stGrpCropInfo.stCropRect);

		pstGrpHwCfg->crop.left = grp_crop.s32X + ctx->s16OffsetLeft;
		pstGrpHwCfg->crop.top = grp_crop.s32Y + ctx->s16OffsetTop;
		pstGrpHwCfg->crop.width = grp_crop.u32Width;
		pstGrpHwCfg->crop.height = grp_crop.u32Height;
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "grp(%d) use frame crop.\n", VpssGrp);
	} else {
		// for grp crop.
		if (ctx->stGrpCropInfo.bEnable) {
			pstGrpHwCfg->crop.width = ctx->stGrpCropInfo.stCropRect.u32Width;
			pstGrpHwCfg->crop.height = ctx->stGrpCropInfo.stCropRect.u32Height;
			pstGrpHwCfg->crop.left = ctx->stGrpCropInfo.stCropRect.s32X +
				ctx->s16OffsetLeft;
			pstGrpHwCfg->crop.top = ctx->stGrpCropInfo.stCropRect.s32Y +
				ctx->s16OffsetTop;
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "grp(%d) use GrpCrop.\n", VpssGrp);
		} else {
			pstGrpHwCfg->crop.left = ctx->s16OffsetLeft;
			pstGrpHwCfg->crop.top = ctx->s16OffsetTop;
			pstGrpHwCfg->crop.width = ctx->stGrpAttr.u32MaxW;
			pstGrpHwCfg->crop.height = ctx->stGrpAttr.u32MaxH;
		}
	}
	memcpy(&pstGrpHwCfg->csc_cfg, &vpssExtCtx[VpssGrp].csc_cfg, sizeof(pstGrpHwCfg->csc_cfg));

	CVI_TRACE_VPSS(CVI_DBG_INFO, "grp(%d) Offset(left:%d top:%d right:%d bottom:%d) rect(%d %d %d %d)\n"
			, VpssGrp, ctx->s16OffsetLeft, ctx->s16OffsetTop, ctx->s16OffsetRight, ctx->s16OffsetBottom
			, pstGrpHwCfg->crop.left, pstGrpHwCfg->crop.top
			, pstGrpHwCfg->crop.width, pstGrpHwCfg->crop.height);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "coef[0][0]: %#4x coef[0][1]: %#4x coef[0][2]: %#4x\n"
		, pstGrpHwCfg->csc_cfg.coef[0][0]
		, pstGrpHwCfg->csc_cfg.coef[0][1]
		, pstGrpHwCfg->csc_cfg.coef[0][2]);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "coef[1][0]: %#4x coef[1][1]: %#4x coef[1][2]: %#4x\n"
		, pstGrpHwCfg->csc_cfg.coef[1][0]
		, pstGrpHwCfg->csc_cfg.coef[1][1]
		, pstGrpHwCfg->csc_cfg.coef[1][2]);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "coef[2][0]: %#4x coef[2][1]: %#4x coef[2][2]: %#4x\n"
		, pstGrpHwCfg->csc_cfg.coef[2][0]
		, pstGrpHwCfg->csc_cfg.coef[2][1]
		, pstGrpHwCfg->csc_cfg.coef[2][2]);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "sub[0]: %3d sub[1]: %3d sub[2]: %3d\n"
		, pstGrpHwCfg->csc_cfg.sub[0]
		, pstGrpHwCfg->csc_cfg.sub[1]
		, pstGrpHwCfg->csc_cfg.sub[2]);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "add[0]: %3d add[1]: %3d add[2]: %3d\n"
		, pstGrpHwCfg->csc_cfg.add[0]
		, pstGrpHwCfg->csc_cfg.add[1]
		, pstGrpHwCfg->csc_cfg.add[2]);

}

static s32 commitHWSettings(struct cvi_vpss_ctx *vpss_ctx)
{
	VPSS_GRP VpssGrp = vpss_ctx->VpssGrp;
	u8 online_from_isp = vpss_ctx->online_from_isp;
	VPSS_GRP_ATTR_S *pstGrpAttr;
	struct VPSS_CHN_CFG *pstChnCfg;
	struct cvi_vpss_grp_cfg *pstHwGrpCfg = &vpss_ctx->hw_cfg.stGrpCfg;
	struct cvi_vpss_chn_cfg *pstHwChnCfg;
	struct vb_s *vb_in = NULL;
	VPSS_CHN VpssChn;
	struct vb_jobs_t *jobs;
	u8 bGrpchanged = CVI_FALSE;
	u8 u8ChnNum = 0, i;
	struct mutex *rgn_mutex;
	struct cvi_rgn_canvas_ctx *rgn_canvas;
	struct cvi_rgn_canvas_q *rgn_canvas_waitq;
	struct cvi_rgn_canvas_q *rgn_canvas_doneq;

	pstGrpAttr = &vpss_ctx->stGrpAttr;

	if (!online_from_isp && _is_frame_crop_changed(VpssGrp, vpss_ctx))
		vpss_ctx->is_cfg_changed = CVI_TRUE; //DIS

	if (vpss_ctx->is_cfg_changed) {
		_vpss_grp_hw_cfg_update(vpss_ctx);
		vpss_ctx->is_cfg_changed = CVI_FALSE;
		bGrpchanged = CVI_TRUE;
	}

	if (!online_from_isp) {
		if (!base_mod_jobs_waitq_empty(&stVpssVbjobs[VpssGrp].ins)) {
			jobs = &stVpssVbjobs[VpssGrp].ins;
			FIFO_GET_FRONT(&jobs->waitq, &vb_in);
		}
	}

	pstHwGrpCfg->online_from_isp = online_from_isp;
	pstHwGrpCfg->src_size.width = pstGrpAttr->u32MaxW;
	pstHwGrpCfg->src_size.height = pstGrpAttr->u32MaxH;
	pstHwGrpCfg->pixelformat = pstGrpAttr->enPixelFormat;
	pstHwGrpCfg->bytesperline[0] = (vb_in != NULL)
				? vb_in->buf.stride[0] : pstHwGrpCfg->bytesperline[0];
	pstHwGrpCfg->bytesperline[1] = (vb_in != NULL)
				? vb_in->buf.stride[1] : pstHwGrpCfg->bytesperline[1];
	pstHwGrpCfg->fbd_enable = (vb_in != NULL)
				? (vb_in->buf.enCompressMode == COMPRESS_MODE_FRAME) : false;
	pstHwGrpCfg->upsample = false;

	for (VpssChn = 0; VpssChn < VPSS_MAX_CHN_NUM; ++VpssChn) {
		pstChnCfg = &vpss_ctx->stChnCfgs[VpssChn];
		pstHwChnCfg = &vpss_ctx->hw_cfg.astChnCfg[VpssChn];
		vpss_ctx->hw_cfg.chn_enable[VpssChn] = pstChnCfg->isEnabled && (!pstChnCfg->isDrop);

		if (!pstChnCfg->isEnabled)
			continue;
		if (pstChnCfg->isDrop)
			continue;
		u8ChnNum++;

		if (bGrpchanged || pstChnCfg->is_cfg_changed) {
			_vpss_chn_hw_cfg_update(VpssChn, vpss_ctx);
			pstChnCfg->is_cfg_changed = CVI_FALSE;
		}

		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "grp(%d) chn(%d) size(%d %d) rect(%d %d %d %d)\n", VpssGrp, VpssChn
				, pstChnCfg->stChnAttr.u32Width, pstChnCfg->stChnAttr.u32Height
				, pstHwChnCfg->crop.left, pstHwChnCfg->crop.top
				, pstHwChnCfg->crop.width, pstHwChnCfg->crop.height);

		pstHwChnCfg->pixelformat = pstChnCfg->stChnAttr.enPixelFormat;
		pstHwChnCfg->src_size.width = pstHwGrpCfg->crop.width;
		pstHwChnCfg->src_size.height = pstHwGrpCfg->crop.height;
		pstHwChnCfg->dst_size.width = pstChnCfg->stChnAttr.u32Width;
		pstHwChnCfg->dst_size.height = pstChnCfg->stChnAttr.u32Height;
		memcpy(pstHwChnCfg->rgn_cfg, pstChnCfg->rgn_cfg, sizeof(pstChnCfg->rgn_cfg));
		for (i = 0; i < RGN_MAX_LAYER_VPSS; ++i) {
			if (pstChnCfg->rgn_cfg[i].rgn_lut_cfg.is_updated) {
				pstChnCfg->rgn_cfg[i].rgn_lut_cfg.is_updated = false;
			}
			if (pstChnCfg->rgn_cfg[i].odec.enable && pstChnCfg->rgn_cfg[i].odec.canvas_updated) {
				rgn_canvas_waitq =
					(struct cvi_rgn_canvas_q *)pstChnCfg->rgn_cfg[i].odec.rgn_canvas_waitq;
				rgn_canvas_doneq =
					(struct cvi_rgn_canvas_q *)pstChnCfg->rgn_cfg[i].odec.rgn_canvas_doneq;
				rgn_mutex = (struct mutex *)pstChnCfg->rgn_cfg[i].odec.canvas_mutex_lock;
				mutex_lock(rgn_mutex);
				if (FIFO_SIZE(rgn_canvas_doneq) != 2) {
					CVI_TRACE_VPSS(CVI_DBG_ERR
						, "grp(%d) chn(%d) rgn layer(%d) doneq size isn't right.\n"
						, VpssGrp, VpssChn, i);
					pstChnCfg->rgn_cfg[i].odec.canvas_updated = false;
					mutex_unlock(rgn_mutex);
				} else {
					FIFO_POP(rgn_canvas_doneq, &rgn_canvas);
					FIFO_PUSH(rgn_canvas_waitq, rgn_canvas);
					pstChnCfg->rgn_cfg[i].odec.canvas_updated = false;
					mutex_unlock(rgn_mutex);
				}
			}
		}


		pstHwChnCfg->rgn_coverex_cfg = pstChnCfg->rgn_coverex_cfg;
		pstHwChnCfg->rgn_mosaic_cfg = pstChnCfg->rgn_mosaic_cfg;

		if (pstChnCfg->stChnAttr.bFlip && pstChnCfg->stChnAttr.bMirror)
			pstHwChnCfg->flip = CVI_SC_FLIP_HVFLIP;
		else if (pstChnCfg->stChnAttr.bFlip)
			pstHwChnCfg->flip = CVI_SC_FLIP_VFLIP;
		else if (pstChnCfg->stChnAttr.bMirror)
			pstHwChnCfg->flip = CVI_SC_FLIP_HFLIP;
		else
			pstHwChnCfg->flip = CVI_SC_FLIP_NO;
		pstHwChnCfg->mute_cfg.enable = pstChnCfg->isMuted;
		pstHwChnCfg->mute_cfg.color[0] = 0;
		pstHwChnCfg->mute_cfg.color[1] = 0;
		pstHwChnCfg->mute_cfg.color[2] = 0;

		if (VPSS_UPSAMPLE(pstGrpAttr->enPixelFormat, pstChnCfg->stChnAttr.enPixelFormat) && (!vpss_ctx->is_copy_upsample))
			pstHwGrpCfg->upsample = true;
	}

	vpss_ctx->hw_cfg.chn_num = u8ChnNum;

	return CVI_SUCCESS;
}

static void _update_vpss_chn_real_frame_rate(struct timer_list *timer)
{
	int i, j;
	u64 duration, curTimeUs;
	struct timespec64 curTime;

	UNUSED(timer);
	ktime_get_ts64(&curTime);
	curTimeUs = (u64)curTime.tv_sec * USEC_PER_SEC + curTime.tv_nsec / NSEC_PER_USEC;

	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (vpssCtx[i] && vpssCtx[i]->isCreated) {
			for (j = 0; j < VPSS_MAX_CHN_NUM; ++j) {
				if (vpssCtx[i]->stChnCfgs[j].isEnabled) {
					duration = curTimeUs - vpssCtx[i]->stChnCfgs[j].stChnWorkStatus.u64PrevTime;
					if (duration >= 1000000) {
						vpssCtx[i]->stChnCfgs[j].stChnWorkStatus.u32RealFrameRate
							= vpssCtx[i]->stChnCfgs[j].stChnWorkStatus.u32FrameNum;
						vpssCtx[i]->stChnCfgs[j].stChnWorkStatus.u32FrameNum = 0;
						vpssCtx[i]->stChnCfgs[j].stChnWorkStatus.u64PrevTime = curTimeUs;
					}
				}
			}
		}
	}
	s_core_cb(s_core_data);

	mod_timer(&timer_proc, jiffies + msecs_to_jiffies(1000));
}

/* _vpss_chl_frame_rate_ctrl: dynamically disabled chn per frame-rate-ctrl
 *
 * @param proc_ctx: the frame statics for reference
 * @param ctx: the working settings
 */
static s32 simplify_rate(u32 dst_in, u32 src_in, u32 *dst_out, u32 *src_out)
{
	u32 i = 1;
	u32 a, b;

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
	return CVI_SUCCESS;
}

static u8 vpss_frame_ctrl(u64 u64FrameIndex, FRAME_RATE_CTRL_S *pstFrameRate)
{
	u32 src_simp;
	u32 dst_simp;
	u32 u32Index;
	u32 srcDur, dstDur;
	u32 curIndx, nextIndx;

	simplify_rate(pstFrameRate->s32DstFrameRate, pstFrameRate->s32SrcFrameRate,
		&dst_simp, &src_simp);

	u32Index = u64FrameIndex % src_simp;
	if (u32Index == 0) {
		return CVI_TRUE;
	}
	srcDur = 100;
	dstDur = (srcDur * src_simp) / dst_simp;
	curIndx = (u32Index - 1) * srcDur / dstDur;
	nextIndx = u32Index * srcDur / dstDur;

	if (nextIndx == curIndx)
		return CVI_FALSE;

	return CVI_TRUE;
}

static u8 _vpss_chl_frame_rate_ctrl(struct cvi_vpss_ctx *ctx, u8 workingMask)
{
	VPSS_CHN VpssChn;

	if (!ctx)
		return 0;
	if (!ctx->isCreated || !ctx->isStarted)
		return 0;

	for (VpssChn = 0; VpssChn < VPSS_MAX_CHN_NUM; ++VpssChn) {
		if (!ctx->stChnCfgs[VpssChn].isEnabled || !ctx->stChnCfgs[VpssChn].stChnWorkStatus.u32SendOk)
			continue;
		ctx->stChnCfgs[VpssChn].isDrop = false;
		if (FRC_INVALID(ctx->stChnCfgs[VpssChn].stChnAttr.stFrameRate))
			continue;
		if (!vpss_frame_ctrl(ctx->stGrpWorkStatus.u32FRCRecvCnt - 1,
			&ctx->stChnCfgs[VpssChn].stChnAttr.stFrameRate)) {
			ctx->stChnCfgs[VpssChn].isDrop = true;
			workingMask &= ~BIT(VpssChn);
			CVI_TRACE_VPSS(CVI_DBG_DEBUG, "chn[%d] frame index(%d) drop\n", VpssChn,
				ctx->stGrpWorkStatus.u32FRCRecvCnt);
		}
	}
	return workingMask;
}

static void _update_vpss_grp_proc(VPSS_GRP VpssGrp, u32 duration, u32 HwDuration)
{
	if (!vpssCtx[VpssGrp])
		return;

	vpssCtx[VpssGrp]->stGrpWorkStatus.u32CostTime = duration;
	if (vpssCtx[VpssGrp]->stGrpWorkStatus.u32MaxCostTime <
		vpssCtx[VpssGrp]->stGrpWorkStatus.u32CostTime) {
		vpssCtx[VpssGrp]->stGrpWorkStatus.u32MaxCostTime
			= vpssCtx[VpssGrp]->stGrpWorkStatus.u32CostTime;
	}
	vpssCtx[VpssGrp]->stGrpWorkStatus.u32HwCostTime = HwDuration;
	if (vpssCtx[VpssGrp]->stGrpWorkStatus.u32HwMaxCostTime <
		vpssCtx[VpssGrp]->stGrpWorkStatus.u32HwCostTime) {
		vpssCtx[VpssGrp]->stGrpWorkStatus.u32HwMaxCostTime
			= vpssCtx[VpssGrp]->stGrpWorkStatus.u32HwCostTime;
	}
}

static void _update_vpss_chn_proc(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
	struct VPSS_CHN_WORK_STATUS_S *pstChnStatus;

	if (!vpssCtx[VpssGrp])
		return;

	pstChnStatus = &vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnWorkStatus;
	pstChnStatus->u32SendOk++;
	pstChnStatus->u32FrameNum++;
}

static u8 _vpss_check_gdc_job(MMF_CHN_S chn, VB_BLK blk, struct cvi_vpss_ctx *vpss_ctx)
{
	struct cvi_gdc_mesh *pmesh;

	pmesh = &mesh[chn.s32DevId][chn.s32ChnId];
	if (mutex_trylock(&pmesh->lock)) {
		if (vpss_ctx->stChnCfgs[chn.s32ChnId].stLDCAttr.bEnable) {
			struct vb_s *vb = (struct vb_s *)blk;
			struct _vpss_gdc_cb_param cb_param = { .chn = chn, .usage = GDC_USAGE_LDC};

			if (_mesh_gdc_do_op_cb(GDC_USAGE_LDC
				, &vpss_ctx->stChnCfgs[chn.s32ChnId].stLDCAttr.stAttr
				, vb
				, vpss_ctx->stChnCfgs[chn.s32ChnId].stChnAttr.enPixelFormat
				, pmesh->paddr
				, CVI_FALSE, &cb_param
				, sizeof(cb_param)
				, CVI_ID_VPSS
				, vpss_ctx->stChnCfgs[chn.s32ChnId].stLDCAttr.stAttr.enRotation) != CVI_SUCCESS) {
				mutex_unlock(&pmesh->lock);
				CVI_TRACE_VPSS(CVI_DBG_ERR, "gdc LDC failed.\n");

				// GDC failed, pass buffer to next module, not block here
				//   e.g. base_get_chn_buffer(-1) blocking
				return CVI_FALSE;
			}
			return CVI_TRUE;
		} else if (vpss_ctx->stChnCfgs[chn.s32ChnId].stFishEyeAttr.bEnable) {
			struct vb_s *vb = (struct vb_s *)blk;
			struct _vpss_gdc_cb_param cb_param = { .chn = chn, .usage = GDC_USAGE_FISHEYE};
			if (_mesh_gdc_do_op_cb(GDC_USAGE_FISHEYE
				, &vpss_ctx->stChnCfgs[chn.s32ChnId].stFishEyeAttr
				, vb
				, vpss_ctx->stChnCfgs[chn.s32ChnId].stChnAttr.enPixelFormat
				, pmesh->paddr
				, CVI_FALSE, &cb_param
				, sizeof(cb_param)
				, CVI_ID_VPSS
				, vpss_ctx->stChnCfgs[chn.s32ChnId].enRotation) != CVI_SUCCESS) {
				mutex_unlock(&pmesh->lock);
				CVI_TRACE_VPSS(CVI_DBG_ERR, "gdc FishEye failed.\n");

				// GDC failed, pass buffer to next module, not block here
				//   e.g. base_get_chn_buffer(-1) blocking
				return CVI_FALSE;
			}
			return CVI_TRUE;
		} else if (vpss_ctx->stChnCfgs[chn.s32ChnId].enRotation != ROTATION_0) {
			struct vb_s *vb = (struct vb_s *)blk;
			struct _vpss_gdc_cb_param cb_param = { .chn = chn,
				.usage = GDC_USAGE_ROTATION };

			if (_mesh_gdc_do_op_cb(GDC_USAGE_ROTATION
				, NULL
				, vb
				, vpss_ctx->stChnCfgs[chn.s32ChnId].stChnAttr.enPixelFormat
				, pmesh->paddr
				, CVI_FALSE, &cb_param
				, sizeof(cb_param)
				, CVI_ID_VPSS
				, vpss_ctx->stChnCfgs[chn.s32ChnId].enRotation) != CVI_SUCCESS) {
				mutex_unlock(&pmesh->lock);
				CVI_TRACE_VPSS(CVI_DBG_ERR, "gdc rotation failed.\n");

				// GDC failed, pass buffer to next module, not block here
				//   e.g. base_get_chn_buffer(-1) blocking
				return CVI_FALSE;
			}
			return CVI_TRUE;
		}
		mutex_unlock(&pmesh->lock);
	} else {
		CVI_TRACE_VPSS(CVI_DBG_WARN, "grp(%d) chn(%d) drop frame due to gdc op blocked.\n",
			chn.s32DevId, chn.s32ChnId);
		// release blk if gdc not done yet
		vb_release_block(blk);
		return CVI_TRUE;
	}

	return CVI_FALSE;
}

static void vpss_online_unprepare(void)
{
	cvi_vpss_hal_online_release_dev();
}

static s32 vpss_online_prepare(VPSS_GRP workingGrp)
{
	u8 i, workingMask = 0;
	struct cvi_vpss_ctx *vpss_ctx = CVI_NULL;
	struct cvi_vpss_job *pastJob[VPSS_ONLINE_JOB_NUM] = {[0 ... VPSS_ONLINE_JOB_NUM - 1] = NULL};

	vpss_ctx = vpssCtx[workingGrp];
	mutex_lock(&vpss_ctx->lock);

	// sc's mask
	workingMask = get_work_mask(vpss_ctx);
	if (workingMask)
		workingMask = _vpss_chl_frame_rate_ctrl(vpss_ctx, workingMask);
	if (workingMask == 0) {
		CVI_TRACE_VPSS(CVI_DBG_NOTICE, "grp(%d) workingMask zero.\n", workingGrp);
		goto err;
	}

	// commit hw settings of this vpss-grp.
	if (commitHWSettings(vpss_ctx) != CVI_SUCCESS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "grp(%d) apply hw settings NG.\n", workingGrp);
		vpss_ctx->stGrpWorkStatus.u32StartFailCnt++;
		goto err;
	}

	for (i = 0; i < VPSS_ONLINE_JOB_NUM; i++) {
		if (FIFO_EMPTY(&vpss_ctx->jobq)) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "vpss(%d) jobq empty.\n", workingGrp);
			goto err;
		}
		FIFO_POP(&vpss_ctx->jobq, &pastJob[i]);
		memcpy(&pastJob[i]->cfg, &vpss_ctx->hw_cfg, sizeof(vpss_ctx->hw_cfg));
		pastJob[i]->workingMask = workingMask;

		if (fill_buffers(vpss_ctx, pastJob[i]) != CVI_SUCCESS) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "grp(%d) fill buffer NG.\n", workingGrp);
			vpss_ctx->stGrpWorkStatus.u32StartFailCnt++;
			goto err;
		}
	}

	ktime_get_ts64(&vpss_ctx->time);
	vpss_ctx->enHdlState = HANDLER_STATE_RUN;
	for (i = 0; i < VPSS_ONLINE_JOB_NUM; i++)
		cvi_vpss_hal_push_job(pastJob[i]);
	mutex_unlock(&vpssCtx[workingGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Online Grp(%d) post job.\n", workingGrp);
	cvi_vpss_hal_try_schedule();

	// wait for h/w done
	return CVI_SUCCESS;

err:
	for (i = 0; i < VPSS_ONLINE_JOB_NUM; i++)
		if (pastJob[i])
			FIFO_PUSH(&vpss_ctx->jobq, pastJob[i]);
	mutex_unlock(&vpss_ctx->lock);

	return CVI_FAILURE;
}

static s32 vpss_online_full_job(VPSS_GRP workingGrp, struct cvi_vpss_job *pstJob)
{
	u8 workingMask = 0;
	struct cvi_vpss_ctx *vpss_ctx = CVI_NULL;

	vpss_ctx = vpssCtx[workingGrp];

	// sc's mask
	workingMask = get_work_mask(vpss_ctx);
	if (workingMask)
		workingMask = _vpss_chl_frame_rate_ctrl(vpss_ctx, workingMask);
	if (workingMask == 0) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "grp(%d) workingMask zero.\n", workingGrp);
		goto err;
	}

	// commit hw settings of this vpss-grp.
	if (commitHWSettings(vpss_ctx) != CVI_SUCCESS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "grp(%d) apply hw settings NG.\n", workingGrp);
		vpss_ctx->stGrpWorkStatus.u32StartFailCnt++;
		goto err;
	}

	memcpy(&pstJob->cfg, &vpss_ctx->hw_cfg, sizeof(vpss_ctx->hw_cfg));
	pstJob->workingMask = workingMask;

	if (fill_buffers(vpss_ctx, pstJob) != CVI_SUCCESS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "grp(%d) fill buffer NG.\n", workingGrp);
		vpss_ctx->stGrpWorkStatus.u32StartFailCnt++;
		goto err;
	}

	ktime_get_ts64(&vpss_ctx->time);
	vpss_ctx->enHdlState = HANDLER_STATE_RUN;
	mutex_unlock(&vpssCtx[workingGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Online Grp(%d) post job.\n", workingGrp);

	// wait for h/w done
	return CVI_SUCCESS;

err:
	return CVI_FAILURE;
}

void vpss_handle_online_frame_done(struct cvi_vpss_job *pstJob)
{
	struct cvi_vpss_ctx *vpss_ctx = (struct cvi_vpss_ctx *)pstJob->data;
	VPSS_GRP workingGrp = pstJob->grp_id;
	u8 workingMask = pstJob->workingMask;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = workingGrp, .s32ChnId = 0};
	VB_BLK blk;
	struct vb_s *vb;
	VPSS_CHN VpssChn;
	u32 duration;
	struct timespec64 time;
	u64 u64PTS;

	if (workingGrp >= VPSS_ONLINE_NUM) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Online Grp(%d) invalid\n", workingGrp);
		return;
	}
	CVI_TRACE_VPSS(CVI_DBG_INFO, "grp(%d) eof\n", workingGrp);
	vpss_ctx->stGrpWorkStatus.u32RecvCnt++;
	vpss_ctx->stGrpWorkStatus.u32FRCRecvCnt++;
	VpssChn = 0;
	ktime_get_ts64(&time);
	duration = get_diff_in_us(vpss_ctx->time, time);

	do {
		if (!(workingMask & BIT(VpssChn)))
			continue;

		if (!vpss_ctx->stChnCfgs[VpssChn].isEnabled)
			continue;

		chn.s32ChnId = VpssChn;

		vb_dqbuf(chn, &stVpssVbjobs[workingGrp].outs[VpssChn], &blk);
		if (blk == VB_INVALID_HANDLE) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Mod(%d) can't get vb-blk.\n", chn.enModId);
			continue;
		}
		//if (vi_ctx.bypass_frm[workingGrp] >= vpssPrcCtx[workingGrp].stChnCfgs[VpssChn].u32SendOk) {
		//	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "grp(%d) chn(%d) drop frame for vi-bypass(%d).\n",
		//		       workingGrp, VpssChn, vi_ctx.bypass_frm[workingGrp]);
		//	vb_release_block(blk);
		//	_update_vpss_chn_proc(workingGrp, VpssChn);
		//	continue;
		//}

		// update pts & frm_num info to vb
		u64PTS = timespec64_to_ns(&time);
		do_div(u64PTS, 1000);
		vb = (struct vb_s *)blk;
		vb->buf.u64PTS = u64PTS;
		vb->buf.dev_num = workingGrp;
		vb->buf.u32FrameFlag = pstJob->checksum[VpssChn];
		vb->buf.frm_num = vpss_ctx->stGrpWorkStatus.u32RecvCnt;
		_vpss_online_set_mlv_info(vb);

		if (_vpss_check_gdc_job(chn, blk, vpss_ctx) != CVI_TRUE)
			vb_done_handler(chn, CHN_TYPE_OUT, &stVpssVbjobs[workingGrp].outs[VpssChn], blk);

		CVI_TRACE_VPSS(CVI_DBG_INFO, "grp(%d) chn(%d) end\n", workingGrp, VpssChn);
		_update_vpss_chn_proc(workingGrp, VpssChn);
	} while (++VpssChn < VPSS_MAX_CHN_NUM);

	// Update vpss grp proc info
	_update_vpss_grp_proc(workingGrp, duration, pstJob->u32HwDuration);

	vpss_online_full_job(workingGrp, pstJob);
	cvi_vpss_hal_push_online_job(pstJob);
}


static void vpss_handle_offline_frame_done(struct cvi_vpss_job *pstJob)
{
	struct cvi_vpss_ctx *vpss_ctx = (struct cvi_vpss_ctx *)pstJob->data;
	VPSS_GRP workingGrp = pstJob->grp_id;
	u8 workingMask = pstJob->workingMask;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = workingGrp, .s32ChnId = 0};
	VB_BLK blk;
	struct vb_s *vb;
	VPSS_CHN VpssChn;
	u32 duration;
	struct timespec64 time;

	CVI_TRACE_VPSS(CVI_DBG_INFO, "grp(%d) eof\n", workingGrp);

	//Todo: spin_lock, vpss destroy?
	vb_dqbuf(chn, &stVpssVbjobs[workingGrp].ins, &blk);
	if (blk == VB_INVALID_HANDLE) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Mod(%d) can't get vb-blk.\n", chn.enModId);
	} else {
		vb_done_handler(chn, CHN_TYPE_IN, &stVpssVbjobs[workingGrp].ins, blk);
	}

	VpssChn = 0;
	do {
		if (!(workingMask & BIT(VpssChn)))
			continue;

		if (!vpss_ctx->stChnCfgs[VpssChn].isEnabled)
			continue;

		chn.s32ChnId = VpssChn;

		vb_dqbuf(chn, &stVpssVbjobs[workingGrp].outs[VpssChn], &blk);
		if (blk == VB_INVALID_HANDLE) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Mod(%d) can't get vb-blk.\n", chn.enModId);
			continue;
		}
		vb = (struct vb_s *)blk;
		vb->buf.u32FrameFlag = pstJob->checksum[VpssChn];
		if (_vpss_check_gdc_job(chn, blk, vpss_ctx) != CVI_TRUE)
			vb_done_handler(chn, CHN_TYPE_OUT, &stVpssVbjobs[workingGrp].outs[VpssChn], blk);

		CVI_TRACE_VPSS(CVI_DBG_INFO, "grp(%d) chn(%d) end\n", workingGrp, VpssChn);
		_update_vpss_chn_proc(workingGrp, VpssChn);
	} while (++VpssChn < VPSS_MAX_CHN_NUM);

	ktime_get_ts64(&time);
	duration = get_diff_in_us(vpss_ctx->time, time);

	// Update vpss grp proc info
	_update_vpss_grp_proc(workingGrp, duration, pstJob->u32HwDuration);
	vpss_ctx->enHdlState = HANDLER_STATE_STOP;
	FIFO_PUSH(&vpss_ctx->jobq, pstJob);
}

static void vpss_handle_frame_done(struct work_struct *work)
{
	struct cvi_vpss_job *pstJob = container_of(work, struct cvi_vpss_job, job_work);
	VPSS_GRP workingGrp = pstJob->grp_id;

	if (!vpssCtx[workingGrp] || !vpssCtx[workingGrp]->isStarted) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) isn't start yet.\n", workingGrp);
		return;
	}
	mutex_lock(&vpssCtx[workingGrp]->lock);

	if (pstJob->is_online)
		vpss_handle_online_frame_done(pstJob);
	else
		vpss_handle_offline_frame_done(pstJob);

	mutex_unlock(&vpssCtx[workingGrp]->lock);
	vpss_notify_wkup_evt();
}

/**
 * @return: 0 if ready
 */
static s32 vpss_try_schedule(u8 workingGrp)
{
	u8 workingMask = 0;
	struct cvi_vpss_ctx *vpss_ctx = CVI_NULL;
	struct cvi_vpss_job *pstJob = NULL;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = workingGrp, .s32ChnId = 0};

	if (!vpssCtx[workingGrp]) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) isn't created yet.\n", workingGrp);
		return -1;
	}

	vpss_ctx = vpssCtx[workingGrp];
	mutex_lock(&vpss_ctx->lock);
	if (!vpss_ctx->isStarted)
		goto vpss_next_job;

	if (FIFO_EMPTY(&vpss_ctx->jobq)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "vpss(%d) jobq empty.\n", workingGrp);
		goto vpss_next_job;
	}

	// sc's mask
	workingMask = get_work_mask(vpss_ctx);
	if (workingMask)
		workingMask = _vpss_chl_frame_rate_ctrl(vpss_ctx, workingMask);
	if (workingMask == 0) {
		CVI_TRACE_VPSS(CVI_DBG_NOTICE, "grp(%d) workingMask zero.\n", workingGrp);
		_release_vpss_waitq(chn, CHN_TYPE_IN);
		goto vpss_next_job;
	}

	// commit hw settings of this vpss-grp.
	if (commitHWSettings(vpss_ctx) != CVI_SUCCESS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "grp(%d) apply hw settings NG.\n", workingGrp);
		_release_vpss_waitq(chn, CHN_TYPE_IN);
		vpss_ctx->stGrpWorkStatus.u32StartFailCnt++;
		goto vpss_next_job;
	}
	FIFO_POP(&vpss_ctx->jobq, &pstJob);
	memcpy(&pstJob->cfg, &vpss_ctx->hw_cfg, sizeof(vpss_ctx->hw_cfg));
	pstJob->workingMask = workingMask;

	if (fill_buffers(vpss_ctx, pstJob) != CVI_SUCCESS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "grp(%d) fill buffer NG.\n", workingGrp);
		vpss_ctx->stGrpWorkStatus.u32StartFailCnt++;
		FIFO_PUSH(&vpss_ctx->jobq, pstJob);
		goto vpss_next_job;
	}

	ktime_get_ts64(&vpss_ctx->time);
	vpss_ctx->enHdlState = HANDLER_STATE_RUN;
	cvi_vpss_hal_push_job(pstJob);
	mutex_unlock(&vpssCtx[workingGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Offline Grp(%d) post job.\n", workingGrp);
	cvi_vpss_hal_try_schedule();

	// wait for h/w done
	return CVI_SUCCESS;

vpss_next_job:
	mutex_unlock(&vpss_ctx->lock);

	return CVI_FAILURE;
}

// static void vpss_timeout(struct cvi_vpss_ctx *ctx)
// {
// 	struct cvi_vpss_job *pstJob = (struct cvi_vpss_job *)ctx->pJobBuffer;

// 	CVI_TRACE_VPSS(CVI_DBG_INFO, "vpss grp(%d) timeout...\n", ctx->VpssGrp);

// 	mutex_lock(&ctx->lock);
// 	cvi_vpss_hal_remove_job(pstJob);
// 	FIFO_PUSH(&ctx->jobq, pstJob);
// 	release_buffers(ctx);
// 	ctx->enHdlState = HANDLER_STATE_STOP;
// 	mutex_unlock(&ctx->lock);
// }

static u8 vpss_handler_is_idle(void)
{
	int i;

	for (i = 0; i < VPSS_MAX_GRP_NUM; i++)
		if (vpssCtx[i] && vpssCtx[i]->isCreated && vpssCtx[i]->isStarted)
			return CVI_FALSE;

	return CVI_TRUE;
}

static int vpss_event_handler(void *arg)
{
	struct vpss_handler_ctx *ctx = (struct vpss_handler_ctx *)arg;
	unsigned long idle_timeout = msecs_to_jiffies(IDLE_TIMEOUT_MS);
	unsigned long eof_timeout = msecs_to_jiffies(EOF_WAIT_TIMEOUT_MS);
	unsigned long timeout = idle_timeout;
	int i, ret;
	struct vb_jobs_t *jobs;
	struct timespec64 time;

	while (!kthread_should_stop()) {
		ret = wait_event_interruptible_timeout(ctx->wait,
			ctx->events || kthread_should_stop(), timeout);

		/* -%ERESTARTSYS */
		if (ret < 0 || kthread_should_stop())
			break;

		/* timeout */
		if (!ret && vpss_handler_is_idle()) {
			timeout = idle_timeout;
			continue;
		}

		//CVI_TRACE_VPSS(CVI_DBG_DEBUG, "vpss thread, events:%d\n", ctx->events);
		spin_lock(&vpss_hdl_ctx.hdl_lock);
		ctx->events &= ~CTX_EVENT_WKUP;
		spin_unlock(&vpss_hdl_ctx.hdl_lock);

		if (!ctx->GrpMask)
			continue;

		ktime_get_ts64(&time);

		for (i = 0; i < VPSS_MAX_GRP_NUM; i++) {
			if (!(ctx->GrpMask & BIT(i)))
				continue;
			if (!vpssCtx[i])
				continue;
			if (vpssCtx[i]->enHdlState == HANDLER_STATE_RUN) {
				// if (get_diff_in_us(vpssCtx[i]->time, time) > (eof_timeout * 1000))
				// 	vpss_timeout(vpssCtx[i]);
				continue;
			}
			jobs = &stVpssVbjobs[i].ins;
			if (!jobs) {
				CVI_TRACE_VPSS(CVI_DBG_INFO, "get jobs failed\n");
				continue;
			}

			if (!down_trylock(&jobs->sem)) {
				vpss_try_schedule(i);
				timeout = eof_timeout;
			}
		}
	}

	return 0;
}


void _vpss_grp_raram_init(VPSS_GRP VpssGrp)
{
	u8 i, j, k;
	PROC_AMP_CTRL_S ctrl;
	struct sclr_csc_matrix *mtrx;

	memset(&vpssCtx[VpssGrp]->stGrpCropInfo, 0, sizeof(vpssCtx[VpssGrp]->stGrpCropInfo));
	memset(&vpssCtx[VpssGrp]->frame_crop, 0, sizeof(vpssCtx[VpssGrp]->frame_crop));
	memset(&vpssCtx[VpssGrp]->stGrpWorkStatus, 0, sizeof(vpssCtx[VpssGrp]->stGrpWorkStatus));

	for (i = 0; i < VPSS_MAX_CHN_NUM; ++i) {
		memset(&vpssCtx[VpssGrp]->stChnCfgs[i], 0, sizeof(vpssCtx[VpssGrp]->stChnCfgs[i]));
		vpssCtx[VpssGrp]->stChnCfgs[i].enCoef = VPSS_SCALE_COEF_BICUBIC;
		vpssCtx[VpssGrp]->stChnCfgs[i].align = DEFAULT_ALIGN;
		vpssCtx[VpssGrp]->stChnCfgs[i].YRatio = YRATIO_SCALE;
		vpssCtx[VpssGrp]->stChnCfgs[i].VbPool = VB_INVALID_POOLID;
		mutex_init(&mesh[VpssGrp][i].lock);

		for (j = 0; j < RGN_MAX_LAYER_VPSS; ++j)
			for (k = 0; k < RGN_MAX_NUM_VPSS; ++k)
				vpssCtx[VpssGrp]->stChnCfgs[i].rgn_handle[j][k] = RGN_INVALID_HANDLE;
		for (j = 0; j < RGN_COVEREX_MAX_NUM; ++j)
			vpssCtx[VpssGrp]->stChnCfgs[i].coverEx_handle[j] = RGN_INVALID_HANDLE;
		for (j = 0; j < RGN_MOSAIC_MAX_NUM; ++j)
			vpssCtx[VpssGrp]->stChnCfgs[i].mosaic_handle[j] = RGN_INVALID_HANDLE;
	}

	for (i = PROC_AMP_BRIGHTNESS; i < PROC_AMP_MAX; ++i) {
		vpss_get_proc_amp_ctrl(i, &ctrl);
		vpssExtCtx[VpssGrp].proc_amp[i] = ctrl.default_value;
	}

	// use designer provided table
	mtrx = sclr_get_csc_mtrx(SCL_CSC_601_LIMIT_YUV2RGB);
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++)
			vpssExtCtx[VpssGrp].csc_cfg.coef[i][j] = mtrx->coef[i][j];

		vpssExtCtx[VpssGrp].csc_cfg.add[i] = mtrx->add[i];
		vpssExtCtx[VpssGrp].csc_cfg.sub[i] = mtrx->sub[i];
	}
	vpssCtx[VpssGrp]->is_copy_upsample = CVI_FALSE;
}

static s32 _vpss_update_rotation_mesh(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, ROTATION_E enRotation)
{
	struct cvi_gdc_mesh *pmesh = &mesh[VpssGrp][VpssChn];

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) rotation(%d).\n",
			VpssGrp, VpssChn, enRotation);

	pmesh->paddr = DEFAULT_MESH_PADDR;

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].enRotation = enRotation;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);
	return CVI_SUCCESS;
}

s32 _vpss_update_ldc_mesh(VPSS_GRP VpssGrp, VPSS_CHN VpssChn,
	const VPSS_LDC_ATTR_S *pstLDCAttr, u64 paddr)
{
	u64 paddr_old;
	struct cvi_gdc_mesh *pmesh = &mesh[VpssGrp][VpssChn];

	mutex_lock(&pmesh->lock);
	if (pmesh->paddr) {
		paddr_old = pmesh->paddr;
	} else {
		paddr_old = 0;
	}
	pmesh->paddr = paddr;
	pmesh->vaddr = NULL;
	mutex_unlock(&pmesh->lock);

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stLDCAttr = *pstLDCAttr;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);
	//mutex_unlock(&pmesh->lock);

	//if (paddr_old)
	//	CVI_SYS_IonFree(paddr_old, vaddr_old);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) mesh base(0x%llx)\n"
		      , VpssGrp, VpssChn, (unsigned long long)paddr);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "bEnable=%d, apect=%d, xyratio=%d, xoffset=%d, yoffset=%d, ratio=%d, rotation=%d\n",
			pstLDCAttr->bEnable, pstLDCAttr->stAttr.bAspect,
			pstLDCAttr->stAttr.s32XYRatio, pstLDCAttr->stAttr.s32CenterXOffset,
			pstLDCAttr->stAttr.s32CenterYOffset, pstLDCAttr->stAttr.s32DistortionRatio,
			pstLDCAttr->stAttr.enRotation);
	return CVI_SUCCESS;
}

s32 _vpss_update_fisheye_mesh(VPSS_GRP VpssGrp, VPSS_CHN VpssChn,
	const FISHEYE_ATTR_S *pstFishEyeAttr, ROTATION_E enRotation, u64 paddr)
{
	u64 paddr_old;
	struct cvi_gdc_mesh *pmesh = &mesh[VpssGrp][VpssChn];

	mutex_lock(&pmesh->lock);
	if (pmesh->paddr) {
		paddr_old = pmesh->paddr;
	} else {
		paddr_old = 0;
	}
	pmesh->paddr = paddr;
	pmesh->vaddr = NULL;
	mutex_unlock(&pmesh->lock);

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stFishEyeAttr = *pstFishEyeAttr;
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].enRotation = enRotation;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);
	//mutex_unlock(&pmesh->lock);

	//if (paddr_old)
	//	CVI_SYS_IonFree(paddr_old, vaddr_old);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) mesh base(0x%llx)\n"
		      , VpssGrp, VpssChn, (unsigned long long)paddr);
	return CVI_SUCCESS;
}

static int vpss_grp_qbuf(MMF_CHN_S chn, VB_BLK blk)
{
	s32 ret;
	VPSS_GRP VpssGrp = chn.s32DevId;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!vpssCtx[VpssGrp]->isStarted) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) not started yet.\n", VpssGrp);
		return CVI_FAILURE;
	}
	if (vpssCtx[VpssGrp]->online_from_isp) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) online, No need to receive buffer.\n", VpssGrp);
		return CVI_FAILURE;
	}
	vpssCtx[VpssGrp]->stGrpWorkStatus.u32RecvCnt++;

	if (!FRC_INVALID(vpssCtx[VpssGrp]->stGrpAttr.stFrameRate) &&
		!vpss_frame_ctrl(vpssCtx[VpssGrp]->stGrpWorkStatus.u32RecvCnt - 1,
		&vpssCtx[VpssGrp]->stGrpAttr.stFrameRate)) {
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "grp[%d] frame index(%d) drop\n", VpssGrp,
			vpssCtx[VpssGrp]->stGrpWorkStatus.u32RecvCnt);
		return CVI_SUCCESS;
	}
	vpssCtx[VpssGrp]->stGrpWorkStatus.u32FRCRecvCnt++;

	ret = vb_qbuf(chn, CHN_TYPE_IN, &stVpssVbjobs[VpssGrp].ins, blk);
	if (ret != CVI_SUCCESS)
		return ret;

	vpss_notify_wkup_evt();

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) qbuf, blk(0x%llx)\n", VpssGrp, blk);

	return CVI_SUCCESS;
}

/**************************************************************************
 *   Public APIs.
 **************************************************************************/
s32 vpss_set_mod_param(const VPSS_MOD_PARAM_S *pstModParam)
{
	s32 ret, i;

	ret = mod_check_null_ptr(CVI_ID_VPSS, pstModParam);
	if (ret != CVI_SUCCESS)
		return ret;
	for (i = 0; i < VPSS_MAX_GRP_NUM; i++)
		if (vpssCtx[i]) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Setting module param must be the first step of VPSS.\n");
			return CVI_ERR_VPSS_NOT_PERM;
		}

	stModParam = *pstModParam;

	return CVI_SUCCESS;
}

s32 vpss_get_mod_param(VPSS_MOD_PARAM_S *pstModParam)
{
	s32 ret;

	ret = mod_check_null_ptr(CVI_ID_VPSS, pstModParam);
	if (ret != CVI_SUCCESS)
		return ret;

	*pstModParam = stModParam;

	return CVI_SUCCESS;
}

s32 vpss_create_grp(VPSS_GRP VpssGrp, const VPSS_GRP_ATTR_S *pstGrpAttr)
{
	u32 u32MinHeight, u32JobNum = 1;
	u8 online_from_isp = CVI_FALSE;
	struct cvi_vpss_job *pstJob;
	s32 ret, i;

	ret = mod_check_null_ptr(CVI_ID_VPSS, pstGrpAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_fmt(VpssGrp, pstGrpAttr->enPixelFormat);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_yuv_param(pstGrpAttr->enPixelFormat, pstGrpAttr->u32MaxW, pstGrpAttr->u32MaxH);
	if (ret != CVI_SUCCESS)
		return ret;

	u32MinHeight = IS_FMT_YUV420(pstGrpAttr->enPixelFormat) ? 2 : 1;
	if ((pstGrpAttr->u32MaxW < VPSS_MIN_IMAGE_WIDTH) || (pstGrpAttr->u32MaxH < u32MinHeight)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) u32MaxW(%d) or u32MaxH(%d) too small\n"
			, VpssGrp, pstGrpAttr->u32MaxW, pstGrpAttr->u32MaxH);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	if (pstGrpAttr->stFrameRate.s32SrcFrameRate < pstGrpAttr->stFrameRate.s32DstFrameRate) {
		CVI_TRACE_VPSS(CVI_DBG_WARN, "Grp(%d) FrameRate ctrl, src(%d) < dst(%d), not support\n"
				, VpssGrp, pstGrpAttr->stFrameRate.s32SrcFrameRate
				, pstGrpAttr->stFrameRate.s32DstFrameRate);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	if (vpssCtx[VpssGrp]) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) is occupied\n", VpssGrp);
		return CVI_ERR_VPSS_EXIST;
	}

	if ((VpssGrp < VPSS_ONLINE_NUM) &&
		((stVIVPSSMode.aenMode[VpssGrp] == VI_OFFLINE_VPSS_ONLINE) ||
		(stVIVPSSMode.aenMode[VpssGrp] == VI_ONLINE_VPSS_ONLINE))) {
		online_from_isp = CVI_TRUE;
		u32JobNum = VPSS_ONLINE_JOB_NUM;
	}

	vpssCtx[VpssGrp] = kzalloc(sizeof(struct cvi_vpss_ctx), GFP_ATOMIC);
	if (!vpssCtx[VpssGrp]) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "vpssCtx kzalloc fail.\n");
		return CVI_ERR_VPSS_NOMEM;
	}

	if (is_bm_scene){
		memset(&job_bm[VpssGrp][0], 0, sizeof(struct cvi_vpss_job));
		vpssCtx[VpssGrp]->pJobBuffer = (void *)&job_bm[VpssGrp][0];
	} else {
		vpssCtx[VpssGrp]->pJobBuffer = kzalloc(sizeof(struct cvi_vpss_job) * u32JobNum, GFP_ATOMIC);
		if (!vpssCtx[VpssGrp]->pJobBuffer) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "job kzalloc fail.\n");
			kfree(vpssCtx[VpssGrp]);
			return CVI_ERR_VPSS_NOMEM;
		}
	}

	if (pstGrpAttr->stFrameRate.s32SrcFrameRate == 0x7fff) // bm scene
		BM_FIFO_INIT(&vpssCtx[VpssGrp]->jobq, u32JobNum, (struct cvi_vpss_job **)&job_bm[VpssGrp][1]);
	else
		FIFO_INIT(&vpssCtx[VpssGrp]->jobq, u32JobNum);

	pstJob = (struct cvi_vpss_job *)vpssCtx[VpssGrp]->pJobBuffer;
	for (i = 0; i < u32JobNum; i++) {
		pstJob[i].grp_id = VpssGrp;
		pstJob[i].is_online = online_from_isp;
		pstJob[i].data = (void *)vpssCtx[VpssGrp];
		pstJob[i].pfnJobCB = vpss_wkup_frame_done_handle;
		INIT_WORK(&pstJob[i].job_work, vpss_handle_frame_done);
		atomic_set(&pstJob[i].enJobState, CVI_JOB_INVALID);
		FIFO_PUSH(&vpssCtx[VpssGrp]->jobq, pstJob + i);
	}

	if (online_from_isp)
		base_mod_jobs_init(&stVpssVbjobs[VpssGrp].ins, 0, 0, 0);
	else if (pstGrpAttr->stFrameRate.s32SrcFrameRate == 0x7fff) // bm scene
		bm_base_mod_jobs_init(VpssGrp, 2, &stVpssVbjobs[VpssGrp].ins, 1, 1, 0);
	else {
		base_mod_jobs_init(&stVpssVbjobs[VpssGrp].ins, 1, 1, 0);
	}

	vpssCtx[VpssGrp]->VpssGrp = VpssGrp;
	vpssCtx[VpssGrp]->isCreated = CVI_TRUE;
	vpssCtx[VpssGrp]->online_from_isp = online_from_isp;
	vpssCtx[VpssGrp]->enHdlState = HANDLER_STATE_STOP;
	mutex_init(&vpssCtx[VpssGrp]->lock);
	memcpy(&vpssCtx[VpssGrp]->stGrpAttr, pstGrpAttr, sizeof(*pstGrpAttr));
	_vpss_grp_raram_init(VpssGrp);
	mutex_lock(&g_VpssLock);
	s_VpssGrpUsed[VpssGrp] = CVI_TRUE;
	mutex_unlock(&g_VpssLock);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) u32MaxW(%d) u32MaxH(%d) PixelFmt(%d) online_from_isp(%d)\n",
		VpssGrp, pstGrpAttr->u32MaxW, pstGrpAttr->u32MaxH,
		pstGrpAttr->enPixelFormat, vpssCtx[VpssGrp]->online_from_isp);
	vpssCtx[VpssGrp]->is_cfg_changed = CVI_TRUE;
	return CVI_SUCCESS;
}

s32 vpss_destroy_grp(VPSS_GRP VpssGrp)
{
	u8 VpssChn;
	s32 ret;
	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;
	if (!vpssCtx[VpssGrp])
		return CVI_SUCCESS;

	// FIXME: free ion until dwa hardware stops
	if (vpssCtx[VpssGrp]->isCreated) {
		mutex_lock(&vpssCtx[VpssGrp]->lock);
		vpssCtx[VpssGrp]->isCreated = CVI_FALSE;
		if (vpssCtx[VpssGrp]->stGrpAttr.stFrameRate.s32SrcFrameRate == 0x7fff) // bm scene
			bm_base_mod_jobs_exit(&stVpssVbjobs[VpssGrp].ins);
		else
			base_mod_jobs_exit(&stVpssVbjobs[VpssGrp].ins);
		for (VpssChn = 0; VpssChn < VPSS_MAX_CHN_NUM; ++VpssChn) {
			vpssCtx[VpssGrp]->stChnCfgs[VpssChn].enRotation = ROTATION_0;
			vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stLDCAttr.bEnable = CVI_FALSE;

			if (mesh[VpssGrp][VpssChn].paddr) {
#if 0
				if (mesh[VpssGrp][VpssChn].paddr && mesh[VpssGrp][VpssChn].paddr != DEFAULT_MESH_PADDR) {
					base_ion_free(mesh[VpssGrp][VpssChn].paddr);
				}
#endif
				mesh[VpssGrp][VpssChn].paddr = 0;
				mesh[VpssGrp][VpssChn].vaddr = 0;
			}
		}
		if (vpssCtx[VpssGrp]->stGrpAttr.stFrameRate.s32SrcFrameRate == 0x7fff) // bm scene
			BM_FIFO_EXIT(&vpssCtx[VpssGrp]->jobq);
		else
			FIFO_EXIT(&vpssCtx[VpssGrp]->jobq);
		mutex_unlock(&vpssCtx[VpssGrp]->lock);
		mutex_destroy(&vpssCtx[VpssGrp]->lock);
	}

	if(!is_bm_scene) // bm scene
		kfree(vpssCtx[VpssGrp]->pJobBuffer);
	kfree(vpssCtx[VpssGrp]);
	vpssCtx[VpssGrp] = NULL;

	mutex_lock(&g_VpssLock);
	s_VpssGrpUsed[VpssGrp] = CVI_FALSE;
	mutex_unlock(&g_VpssLock);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d)\n", VpssGrp);
	return CVI_SUCCESS;
}

VPSS_GRP vpss_get_available_grp(void)
{
	VPSS_GRP grp = 0;
	VPSS_GRP ret = VPSS_INVALID_GRP;
	u8 i;

	for (i = 0; i < VPSS_ONLINE_NUM; i++) {
		if ((stVIVPSSMode.aenMode[i] == VI_ONLINE_VPSS_ONLINE) ||
			(stVIVPSSMode.aenMode[i] == VI_OFFLINE_VPSS_ONLINE))
			grp = VPSS_ONLINE_NUM;
	}

	mutex_lock(&g_VpssLock);
	for (; grp < VPSS_MAX_GRP_NUM; ++grp)
		if (!s_VpssGrpUsed[grp]) {
			s_VpssGrpUsed[grp] = CVI_TRUE;
			ret = grp;
			break;
		}
	mutex_unlock(&g_VpssLock);

	if ((VPSS_INVALID_GRP != ret) && vpssCtx[ret]){
		vpss_disable_chn(ret, 0);
		vpss_stop_grp(ret);
		vpss_destroy_grp(ret);
		mutex_lock(&g_VpssLock);
		s_VpssGrpUsed[ret] = CVI_TRUE;
		mutex_unlock(&g_VpssLock);
	}

	return ret;
}

s32 vpss_start_grp(VPSS_GRP VpssGrp)
{
	s32 ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	if (vpssCtx[VpssGrp]->isStarted) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) already started.\n", VpssGrp);
		return CVI_SUCCESS;
	}

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->isStarted = CVI_TRUE;
	vpssCtx[VpssGrp]->enHdlState = HANDLER_STATE_STOP;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	if (vpssCtx[VpssGrp]->online_from_isp) {
		ret = vpss_online_prepare(VpssGrp);
		if (ret != CVI_SUCCESS) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) vpss_online_prepare failed.\n", VpssGrp);
			vpssCtx[VpssGrp]->isStarted = CVI_FALSE;
			return ret;
		}
	} else {
		spin_lock(&vpss_hdl_ctx.hdl_lock);
		vpss_hdl_ctx.GrpMask |= BIT(VpssGrp);
		spin_unlock(&vpss_hdl_ctx.hdl_lock);
	}
	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d)\n", VpssGrp);

	return CVI_SUCCESS;
}

s32 vpss_stop_grp(VPSS_GRP VpssGrp)
{
	s32 ret, i; //Todo: online ???
	struct cvi_vpss_job *pstJob;
	u32 u32JobNum;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!vpssCtx[VpssGrp])
		return CVI_SUCCESS;
	if (!vpssCtx[VpssGrp]->isStarted)
		return CVI_SUCCESS;

	spin_lock(&vpss_hdl_ctx.hdl_lock);
	vpss_hdl_ctx.GrpMask &= ~BIT(VpssGrp);
	spin_unlock(&vpss_hdl_ctx.hdl_lock);

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->isStarted = CVI_FALSE;

	u32JobNum = vpssCtx[VpssGrp]->online_from_isp ? VPSS_ONLINE_JOB_NUM : 1;
	pstJob = (struct cvi_vpss_job *)vpssCtx[VpssGrp]->pJobBuffer;

	for (i = 0; i < u32JobNum; i++) {
		if ((atomic_read(&pstJob[i].enJobState) == CVI_JOB_WAIT) ||
			(atomic_read(&pstJob[i].enJobState) == CVI_JOB_WORKING)) {
			cvi_vpss_hal_remove_job(pstJob + i);
			FIFO_PUSH(&vpssCtx[VpssGrp]->jobq, pstJob + i);
			release_buffers(vpssCtx[VpssGrp]);
		}
		atomic_set(&pstJob[i].enJobState, CVI_JOB_INVALID);
		cancel_work_sync(&pstJob[i].job_work);
	}

	vpssCtx[VpssGrp]->enHdlState = HANDLER_STATE_STOP;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	if (vpssCtx[VpssGrp]->online_from_isp && !vpss_online_is_idle())
		vpss_online_unprepare();

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d)\n", VpssGrp);

	return CVI_SUCCESS;
}

s32 vpss_reset_grp(VPSS_GRP VpssGrp)
{
	s32 ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	vpss_stop_grp(VpssGrp);
	mutex_lock(&vpssCtx[VpssGrp]->lock);
	memset(vpssCtx[VpssGrp]->stChnCfgs, 0, sizeof(struct VPSS_CHN_CFG) * VPSS_MAX_CHN_NUM);
	memset(&vpssExtCtx[VpssGrp], 0, sizeof(vpssExtCtx[VpssGrp]));
	_vpss_grp_raram_init(VpssGrp);
	vpssCtx[VpssGrp]->is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d)\n", VpssGrp);

	return CVI_SUCCESS;
}

s32 vpss_get_grp_attr(VPSS_GRP VpssGrp, VPSS_GRP_ATTR_S *pstGrpAttr)
{
	s32 ret;

	ret = mod_check_null_ptr(CVI_ID_VPSS, pstGrpAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	*pstGrpAttr = vpssCtx[VpssGrp]->stGrpAttr;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	return CVI_SUCCESS;
}

s32 vpss_set_grp_attr(VPSS_GRP VpssGrp, const VPSS_GRP_ATTR_S *pstGrpAttr)
{
	u32 u32MinHeight;
	s32 ret;

	ret = mod_check_null_ptr(CVI_ID_VPSS, pstGrpAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_fmt(VpssGrp, pstGrpAttr->enPixelFormat);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_yuv_param(pstGrpAttr->enPixelFormat, pstGrpAttr->u32MaxW, pstGrpAttr->u32MaxH);
	if (ret != CVI_SUCCESS)
		return ret;

	u32MinHeight = IS_FMT_YUV420(pstGrpAttr->enPixelFormat) ? 2 : 1;
	if ((pstGrpAttr->u32MaxW < VPSS_MIN_IMAGE_WIDTH) || (pstGrpAttr->u32MaxH < u32MinHeight)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) u32MaxW(%d) or u32MaxH(%d) too small\n"
			, VpssGrp, pstGrpAttr->u32MaxW, pstGrpAttr->u32MaxH);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	if (vpssCtx[VpssGrp]->online_from_isp && vpssCtx[VpssGrp]->isStarted) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) operation not allowed if vi-2-vpss online\n", VpssGrp);
		return CVI_ERR_VPSS_NOT_PERM;
	}
	if (pstGrpAttr->stFrameRate.s32SrcFrameRate < pstGrpAttr->stFrameRate.s32DstFrameRate) {
		CVI_TRACE_VPSS(CVI_DBG_WARN, "Grp(%d) FrameRate ctrl, src(%d) < dst(%d), not support\n"
				, VpssGrp, pstGrpAttr->stFrameRate.s32SrcFrameRate
				, pstGrpAttr->stFrameRate.s32DstFrameRate);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stGrpAttr = *pstGrpAttr;
	vpssCtx[VpssGrp]->is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) u32MaxW(%d) u32MaxH(%d) PixelFmt(%d) online_from_isp(%d)\n",
		VpssGrp, pstGrpAttr->u32MaxW, pstGrpAttr->u32MaxH,
		pstGrpAttr->enPixelFormat, vpssCtx[VpssGrp]->online_from_isp);
	return CVI_SUCCESS;
}

s32 vpss_set_grp_csc(struct vpss_grp_csc_cfg *cfg)
{
	s32 ret;
	VPSS_GRP VpssGrp = cfg->VpssGrp;
	u8 i, j;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++)
			vpssExtCtx[VpssGrp].csc_cfg.coef[i][j] = cfg->coef[i][j];

		vpssExtCtx[VpssGrp].csc_cfg.add[i] = cfg->add[i];
		vpssExtCtx[VpssGrp].csc_cfg.sub[i] = cfg->sub[i];
	}
	for (i = PROC_AMP_BRIGHTNESS; i < PROC_AMP_MAX; ++i)
		vpssExtCtx[VpssGrp].proc_amp[i] = cfg->proc_amp[i];
	vpssCtx[VpssGrp]->is_copy_upsample = cfg->is_copy_upsample;
	vpssCtx[VpssGrp]->is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d)\n", VpssGrp);

	return CVI_SUCCESS;
}

s32 vpss_set_chn_csc(struct vpss_chn_csc_cfg *cfg)
{
	s32 ret;
	VPSS_GRP VpssGrp = cfg->VpssGrp;
	VPSS_CHN VpssChn = cfg->VpssChn;
	u8 i, j;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++)
			vpssExtCtx[VpssGrp].chn_csc_cfg[VpssChn].coef[i][j] = cfg->coef[i][j];

		vpssExtCtx[VpssGrp].chn_csc_cfg[VpssChn].add[i] = cfg->add[i];
		vpssExtCtx[VpssGrp].chn_csc_cfg[VpssChn].sub[i] = cfg->sub[i];
	}
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);

	return CVI_SUCCESS;
}

s32 vpss_get_proc_amp_ctrl(PROC_AMP_E type, PROC_AMP_CTRL_S *ctrl)
{
	mod_check_null_ptr(CVI_ID_VPSS, ctrl);

	if (type >= PROC_AMP_MAX) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "ProcAmp type(%d) invalid.\n", type);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	*ctrl = procamp_ctrls[type];
	return CVI_SUCCESS;
}

s32 vpss_get_proc_amp(VPSS_GRP VpssGrp, s32 *proc_amp)
{
	s32 ret;
	u8 i;

	mod_check_null_ptr(CVI_ID_VPSS, proc_amp);

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	for (i = PROC_AMP_BRIGHTNESS; i < PROC_AMP_MAX; ++i)
		proc_amp[i] = vpssExtCtx[VpssGrp].proc_amp[i];

	return CVI_SUCCESS;
}

s32 vpss_get_all_proc_amp(struct vpss_all_proc_amp_cfg *cfg)
{
	u8 i, j;

	mod_check_null_ptr(CVI_ID_VPSS, cfg);

	for (i = 0; i < VPSS_MAX_GRP_NUM; i++)
		for (j = PROC_AMP_BRIGHTNESS; j < PROC_AMP_MAX; ++j)
			cfg->proc_amp[i][j] = vpssExtCtx[i].proc_amp[j];

	return CVI_SUCCESS;
}

s32 vpss_set_chn_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_CHN_ATTR_S *pstChnAttr)
{
	VB_CAL_CONFIG_S stVbCalConfig;
	struct sclr_csc_matrix *mtrx;
	u32 u32MinHeight;
	s32 ret;
	u8 i, j;

	ret = mod_check_null_ptr(CVI_ID_VPSS, pstChnAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_fmt(VpssGrp, VpssChn, pstChnAttr->enPixelFormat);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_yuv_param(pstChnAttr->enPixelFormat, pstChnAttr->u32Width, pstChnAttr->u32Height);
	if (ret != CVI_SUCCESS)
		return ret;

	u32MinHeight = IS_FMT_YUV420(pstChnAttr->enPixelFormat) ? 2 : 1;
	if ((pstChnAttr->u32Width < VPSS_MIN_IMAGE_WIDTH) || (pstChnAttr->u32Height < u32MinHeight)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) u32Width(%d) or u32Height(%d) too small\n"
			, VpssGrp, VpssChn, pstChnAttr->u32Width, pstChnAttr->u32Height);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	if (vpssCtx[VpssGrp]->online_from_isp && vpssCtx[VpssGrp]->isStarted) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "You must stop the group before changing the channel attributes,Grp(%d)\n",
			VpssGrp);
		return CVI_ERR_VPSS_NOT_PERM;
	}

	if (pstChnAttr->stAspectRatio.enMode == ASPECT_RATIO_MANUAL) {
		const RECT_S *rect = &pstChnAttr->stAspectRatio.stVideoRect;

		if (!pstChnAttr->stAspectRatio.bEnableBgColor) {
			ret = check_yuv_param(pstChnAttr->enPixelFormat, rect->u32Width, rect->u32Height);
			if (ret != CVI_SUCCESS)
				return ret;
			if ((IS_FMT_YUV420(pstChnAttr->enPixelFormat) || IS_FMT_YUV422(pstChnAttr->enPixelFormat))
				&& (rect->s32X & 0x01)) {
				CVI_TRACE_VPSS(CVI_DBG_ERR, "ASPECT_RATIO_MANUAL invalid.\n");
				CVI_TRACE_VPSS(CVI_DBG_ERR, "YUV_420/YUV_422 rect s32X(%d) should be even.\n",
					rect->s32X);
				return CVI_ERR_VPSS_ILLEGAL_PARAM;
			}
		}

		if ((rect->s32X < 0) || (rect->s32Y < 0)) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "ASPECT_RATIO_MANUAL invalid.\n");
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) rect pos(%d %d) can't be negative.\n"
				, VpssGrp, VpssChn, rect->s32X, rect->s32Y);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}

		if ((rect->u32Width < 4) || (rect->u32Height < u32MinHeight)) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "ASPECT_RATIO_MANUAL invalid.\n");
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) rect size(%d %d) can't smaller than 4x%d\n"
				, VpssGrp, VpssChn, rect->u32Width, rect->u32Height, u32MinHeight);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}

		if ((rect->s32X + rect->u32Width > pstChnAttr->u32Width)
		|| (rect->s32Y + rect->u32Height > pstChnAttr->u32Height)) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "ASPECT_RATIO_MANUAL invalid.\n");
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) rect(%d %d %d %d) output-size(%d %d).\n"
					, VpssGrp, VpssChn
					, rect->s32X, rect->s32Y, rect->u32Width, rect->u32Height
					, pstChnAttr->u32Width, pstChnAttr->u32Height);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	if (pstChnAttr->stFrameRate.s32SrcFrameRate < pstChnAttr->stFrameRate.s32DstFrameRate) {
		CVI_TRACE_VPSS(CVI_DBG_WARN, "Grp(%d) Chn(%d) FrameRate ctrl, src(%d) < dst(%d), not support\n"
				, VpssGrp, VpssChn, pstChnAttr->stFrameRate.s32SrcFrameRate
				, pstChnAttr->stFrameRate.s32DstFrameRate);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	COMMON_GetPicBufferConfig(pstChnAttr->u32Width, pstChnAttr->u32Height,
		pstChnAttr->enPixelFormat, DATA_BITWIDTH_8
		, COMPRESS_MODE_NONE, DEFAULT_ALIGN, &stVbCalConfig);

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	memcpy(&vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr, pstChnAttr,
		sizeof(vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr));
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].blk_size = stVbCalConfig.u32VBSize;
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].align = DEFAULT_ALIGN;
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	// use designer provided table
	if(pstChnAttr->enPixelFormat == PIXEL_FORMAT_YUV_400)
		mtrx = sclr_get_csc_mtrx(SCL_CSC_NONE);
	else
		mtrx = sclr_get_csc_mtrx(SCL_CSC_601_LIMIT_RGB2YUV);

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++)
			vpssExtCtx[VpssGrp].chn_csc_cfg[VpssChn].coef[i][j] = mtrx->coef[i][j];

		vpssExtCtx[VpssGrp].chn_csc_cfg[VpssChn].add[i] = mtrx->add[i];
		vpssExtCtx[VpssGrp].chn_csc_cfg[VpssChn].sub[i] = mtrx->sub[i];
	}

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) Chn(%d) u32Width(%d), u32Height(%d)\n"
		, VpssGrp, VpssChn, pstChnAttr->u32Width, pstChnAttr->u32Height);

	return CVI_SUCCESS;
}

s32 vpss_get_chn_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_CHN_ATTR_S *pstChnAttr)
{
	s32 ret;

	ret = mod_check_null_ptr(CVI_ID_VPSS, pstChnAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	memcpy(pstChnAttr, &vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr, sizeof(*pstChnAttr));
	return CVI_SUCCESS;
}

s32 vpss_enable_chn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
	struct VPSS_CHN_CFG *chn_cfg;
	u32 ret;
	VPSS_GRP grp;
	VPSS_CHN chn;
	u8 u8ChnNum;
	u8 u8MaxChnNum = 0;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!vpssCtx[VpssGrp]->online_from_isp && VpssChn >= 2) {
		for (grp = 0; grp < VPSS_ONLINE_NUM; grp++) {
			if (vpssCtx[grp] && vpssCtx[grp]->isCreated && vpssCtx[grp]->online_from_isp) {
				u8ChnNum = 0;
				for (chn = 0; chn < VPSS_MAX_CHN_NUM; chn++) {
					if (vpssCtx[grp]->stChnCfgs[chn].isEnabled)
						u8ChnNum++;
				}
				u8MaxChnNum = u8MaxChnNum > u8ChnNum ? u8MaxChnNum : u8ChnNum;
			}
		}

		if (VpssChn + u8MaxChnNum >= VPSS_MAX_CHN_NUM) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) invalid channel ID. Job maximum chn ID(%d)\n",
				VpssGrp, VpssChn, VPSS_MAX_CHN_NUM - u8MaxChnNum - 1);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	chn_cfg = &vpssCtx[VpssGrp]->stChnCfgs[VpssChn];
	if (chn_cfg->isEnabled) {
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) already enabled\n", VpssGrp, VpssChn);
		return CVI_SUCCESS;
	}

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	if (vpssCtx[VpssGrp]->online_from_isp)
		base_mod_jobs_init(&stVpssVbjobs[VpssGrp].outs[VpssChn], 1, VPSS_ONLINE_JOB_NUM, chn_cfg->stChnAttr.u32Depth);
	else if (vpssCtx[VpssGrp]->stGrpAttr.stFrameRate.s32SrcFrameRate == 0x7fff) // bm scene
		bm_base_mod_jobs_init(VpssGrp, 4, &stVpssVbjobs[VpssGrp].outs[VpssChn], 1, 1, chn_cfg->stChnAttr.u32Depth);
	else
		base_mod_jobs_init(&stVpssVbjobs[VpssGrp].outs[VpssChn], 1, 1, chn_cfg->stChnAttr.u32Depth);
	chn_cfg->isEnabled = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);

	return CVI_SUCCESS;
}

s32 vpss_disable_chn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
	s32 ret;
	struct VPSS_CHN_WORK_STATUS_S *pstChnStatus;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!vpssCtx[VpssGrp]->isCreated) {
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) not created yet\n", VpssGrp);
		return CVI_SUCCESS;
	}
	if (!vpssCtx[VpssGrp]->stChnCfgs[VpssChn].isEnabled) {
		CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) not enabled yet\n", VpssGrp, VpssChn);
		return CVI_SUCCESS;
	}

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].isEnabled = CVI_FALSE;
	pstChnStatus = &vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnWorkStatus;
	pstChnStatus->u32SendOk = 0;
	pstChnStatus->u64PrevTime = 0;
	pstChnStatus->u32FrameNum = 0;
	pstChnStatus->u32RealFrameRate = 0;
	if (vpssCtx[VpssGrp]->stGrpAttr.stFrameRate.s32SrcFrameRate == 0x7fff) // bm scene
		bm_base_mod_jobs_exit(&stVpssVbjobs[VpssGrp].outs[VpssChn]);
	else
		base_mod_jobs_exit(&stVpssVbjobs[VpssGrp].outs[VpssChn]);
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);

	return CVI_SUCCESS;
}

s32 vpss_set_chn_crop(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_CROP_INFO_S *pstCropInfo)
{
	s32 ret;

	ret = mod_check_null_ptr(CVI_ID_VPSS, pstCropInfo);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (pstCropInfo->bEnable) {
		if ((pstCropInfo->stCropRect.u32Width < 4) || (pstCropInfo->stCropRect.u32Height < 1)) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) crop size(%d %d) can't smaller than 4x1\n"
				, VpssGrp, VpssChn, pstCropInfo->stCropRect.u32Width
				, pstCropInfo->stCropRect.u32Height);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}

		if (pstCropInfo->stCropRect.s32X + (s32)pstCropInfo->stCropRect.u32Width < 4
			|| pstCropInfo->stCropRect.s32Y + (s32)pstCropInfo->stCropRect.u32Height < 1) {
			CVI_TRACE_VPSS(CVI_DBG_ERR
				, "Grp(%d) Chn(%d) crop rect(%d %d %d %d) can't smaller than 4x1\n"
				, VpssGrp, VpssChn, pstCropInfo->stCropRect.s32X, pstCropInfo->stCropRect.s32Y
				, pstCropInfo->stCropRect.u32Width, pstCropInfo->stCropRect.u32Height);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}

		if (vpssCtx[VpssGrp]->stGrpCropInfo.bEnable) {
			if ((s32)vpssCtx[VpssGrp]->stGrpCropInfo.stCropRect.u32Height
				- pstCropInfo->stCropRect.s32Y < 1
				|| (s32)vpssCtx[VpssGrp]->stGrpCropInfo.stCropRect.u32Width
				- pstCropInfo->stCropRect.s32X < 4) {
				CVI_TRACE_VPSS(CVI_DBG_ERR
					, "Grp(%d) Chn(%d) crop rect(%d %d %d %d) can't smaller than 4x1\n"
					, VpssGrp, VpssChn, pstCropInfo->stCropRect.s32X, pstCropInfo->stCropRect.s32Y
					, pstCropInfo->stCropRect.u32Width, pstCropInfo->stCropRect.u32Height);
				CVI_TRACE_VPSS(CVI_DBG_ERR, "grp crop size(%d %d)\n"
					, vpssCtx[VpssGrp]->stGrpCropInfo.stCropRect.u32Width
					, vpssCtx[VpssGrp]->stGrpCropInfo.stCropRect.u32Height);
				return CVI_ERR_VPSS_ILLEGAL_PARAM;
			}
		} else {
			if ((s32)vpssCtx[VpssGrp]->stGrpAttr.u32MaxH - pstCropInfo->stCropRect.s32Y < 1
				|| (s32)vpssCtx[VpssGrp]->stGrpAttr.u32MaxW - pstCropInfo->stCropRect.s32X < 4) {
				CVI_TRACE_VPSS(CVI_DBG_ERR
					, "Grp(%d) Chn(%d) crop rect(%d %d %d %d) can't smaller than 4x1\n"
					, VpssGrp, VpssChn, pstCropInfo->stCropRect.s32X, pstCropInfo->stCropRect.s32Y
					, pstCropInfo->stCropRect.u32Width, pstCropInfo->stCropRect.u32Height);
				CVI_TRACE_VPSS(CVI_DBG_ERR, "out of grp size(%d %d)\n"
					, vpssCtx[VpssGrp]->stGrpAttr.u32MaxW, vpssCtx[VpssGrp]->stGrpAttr.u32MaxH);
				return CVI_ERR_VPSS_ILLEGAL_PARAM;
			}
		}
	}

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stCropInfo = *pstCropInfo;
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);
	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) Chn(%d), bEnable=%d, rect(%d %d %d %d)\n",
		VpssGrp, VpssChn, pstCropInfo->bEnable,
		pstCropInfo->stCropRect.s32X, pstCropInfo->stCropRect.s32Y,
		pstCropInfo->stCropRect.u32Width, pstCropInfo->stCropRect.u32Height);

	return CVI_SUCCESS;
}

s32 vpss_get_chn_crop(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_CROP_INFO_S *pstCropInfo)
{
	s32 ret;

	ret = mod_check_null_ptr(CVI_ID_VPSS, pstCropInfo);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	*pstCropInfo = vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stCropInfo;

	return CVI_SUCCESS;
}

s32 vpss_show_chn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
	s32 ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].isMuted = CVI_FALSE;
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);
	return CVI_SUCCESS;
}

s32 vpss_hide_chn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
	s32 ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!vpssCtx[VpssGrp])
		return CVI_SUCCESS;
	if (!vpssCtx[VpssGrp]->stChnCfgs[VpssChn].isEnabled)
		return CVI_SUCCESS;

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].isMuted = CVI_TRUE;
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);
	return CVI_SUCCESS;
}

s32 vpss_get_grp_crop(VPSS_GRP VpssGrp, VPSS_CROP_INFO_S *pstCropInfo)
{
	s32 ret;

	ret = mod_check_null_ptr(CVI_ID_VPSS, pstCropInfo);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	*pstCropInfo = vpssCtx[VpssGrp]->stGrpCropInfo;
	return CVI_SUCCESS;
}

s32 vpss_set_grp_crop(VPSS_GRP VpssGrp, const VPSS_CROP_INFO_S *pstCropInfo)
{
	s32 ret;

	ret = mod_check_null_ptr(CVI_ID_VPSS, pstCropInfo);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	if (vpssCtx[VpssGrp]->online_from_isp) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) not support crop if online\n", VpssGrp);
		return CVI_ERR_VPSS_NOT_PERM;
	}

	if (pstCropInfo->bEnable) {
		u32 u32MinHeight = IS_FMT_YUV420(vpssCtx[VpssGrp]->stGrpAttr.enPixelFormat)
				     ? 2 : 1;

		if ((pstCropInfo->stCropRect.s32X < 0) || (pstCropInfo->stCropRect.s32Y < 0)) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) crop start-point(%d %d) illegal\n"
				, VpssGrp, pstCropInfo->stCropRect.s32X, pstCropInfo->stCropRect.s32Y);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}

		if ((pstCropInfo->stCropRect.u32Width < 4) || (pstCropInfo->stCropRect.u32Height < u32MinHeight)) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) crop size(%d %d) can't smaller than 4x%d\n"
				, VpssGrp, pstCropInfo->stCropRect.u32Width, pstCropInfo->stCropRect.u32Height
				, u32MinHeight);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}

		if ((pstCropInfo->stCropRect.s32Y + pstCropInfo->stCropRect.u32Height)
			> vpssCtx[VpssGrp]->stGrpAttr.u32MaxH
		 || (pstCropInfo->stCropRect.s32X + pstCropInfo->stCropRect.u32Width)
			 > vpssCtx[VpssGrp]->stGrpAttr.u32MaxW) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) crop rect(%d %d %d %d) out of grp size(%d %d)\n"
				, VpssGrp, pstCropInfo->stCropRect.s32X, pstCropInfo->stCropRect.s32Y
				, pstCropInfo->stCropRect.u32Width, pstCropInfo->stCropRect.u32Height
				, vpssCtx[VpssGrp]->stGrpAttr.u32MaxW, vpssCtx[VpssGrp]->stGrpAttr.u32MaxH);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stGrpCropInfo = *pstCropInfo;
	if (pstCropInfo->bEnable) {
		bool chk_width_even = IS_FMT_YUV420(vpssCtx[VpssGrp]->stGrpAttr.enPixelFormat) ||
				      IS_FMT_YUV422(vpssCtx[VpssGrp]->stGrpAttr.enPixelFormat);
		bool chk_height_even = IS_FMT_YUV420(vpssCtx[VpssGrp]->stGrpAttr.enPixelFormat);

		if (chk_width_even && (pstCropInfo->stCropRect.u32Width & 0x01)) {
			vpssCtx[VpssGrp]->stGrpCropInfo.stCropRect.u32Width &= ~(0x0001);
			CVI_TRACE_VPSS(CVI_DBG_WARN, "Grp(%d) stCropRect.u32Width(%d) to even(%d) due to YUV\n",
				       VpssGrp, pstCropInfo->stCropRect.u32Width,
				       vpssCtx[VpssGrp]->stGrpCropInfo.stCropRect.u32Width);
		}
		if (chk_height_even && (pstCropInfo->stCropRect.u32Height & 0x01)) {
			vpssCtx[VpssGrp]->stGrpCropInfo.stCropRect.u32Height &= ~(0x0001);
			CVI_TRACE_VPSS(CVI_DBG_WARN, "Grp(%d) stCropRect.u32Height(%d) to even(%d) due to YUV\n",
				       VpssGrp, pstCropInfo->stCropRect.u32Height,
				       vpssCtx[VpssGrp]->stGrpCropInfo.stCropRect.u32Height);
		}
	}

	vpssCtx[VpssGrp]->is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_INFO, "Grp(%d), bEnable=%d, rect(%d %d %d %d)\n",
		VpssGrp, pstCropInfo->bEnable,
		pstCropInfo->stCropRect.s32X, pstCropInfo->stCropRect.s32Y,
		pstCropInfo->stCropRect.u32Width, pstCropInfo->stCropRect.u32Height);

	return CVI_SUCCESS;
}

//TBD
s32 vpss_get_grp_frame(VPSS_GRP VpssGrp, VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	s32 ret;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d)\n", VpssGrp);

	ret = mod_check_null_ptr(CVI_ID_VPSS, pstVideoFrame);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	return CVI_SUCCESS;
}

s32 vpss_release_grp_frame(VPSS_GRP VpssGrp, const VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	s32 ret;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d)\n", VpssGrp);

	ret = mod_check_null_ptr(CVI_ID_VPSS, pstVideoFrame);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	return CVI_SUCCESS;
}

VB_BLK vb_create_block_vpss(struct vb_s *p, uint64_t phyAddr, void *virAddr, VB_POOL VbPool, bool isExternal)
{
	p->phy_addr = phyAddr;
	p->vir_addr = virAddr;
	p->vb_pool = VbPool;
	atomic_set(&p->usr_cnt, 10);
	p->magic = CVI_VB_MAGIC;
	atomic_long_set(&p->mod_ids, 0);
	p->external = isExternal;

	return (VB_BLK)p;
}

s32 vpss_send_frame(VPSS_GRP VpssGrp, const VIDEO_FRAME_INFO_S *pstVideoFrame, s32 s32MilliSec)
{
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = 0};
	VB_BLK blk;
	s32 ret;

	ret = mod_check_null_ptr(CVI_ID_VPSS, pstVideoFrame);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	if (vpssCtx[VpssGrp]->online_from_isp) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) not support if online\n", VpssGrp);
		return CVI_ERR_VPSS_NOT_PERM;
	}

	if (!vpssCtx[VpssGrp]->isStarted) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) not yet started.\n", VpssGrp);
		return CVI_ERR_VPSS_NOTREADY;
	}
	if (vpssCtx[VpssGrp]->stGrpAttr.enPixelFormat != pstVideoFrame->stVFrame.enPixelFormat) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) PixelFormat(%d) mismatch.\n"
			, VpssGrp, pstVideoFrame->stVFrame.enPixelFormat);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}
	if ((vpssCtx[VpssGrp]->stGrpAttr.u32MaxW != pstVideoFrame->stVFrame.u32Width)
	 || (vpssCtx[VpssGrp]->stGrpAttr.u32MaxH != pstVideoFrame->stVFrame.u32Height)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Size(%d * %d) mismatch.\n"
			, VpssGrp, pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.u32Height);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}
	if (IS_FRAME_OFFSET_INVALID(pstVideoFrame->stVFrame)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) frame offset (%d %d %d %d) invalid\n",
			VpssGrp, pstVideoFrame->stVFrame.s16OffsetLeft, pstVideoFrame->stVFrame.s16OffsetRight,
			pstVideoFrame->stVFrame.s16OffsetTop, pstVideoFrame->stVFrame.s16OffsetBottom);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}
	if (IS_FMT_YUV420(vpssCtx[VpssGrp]->stGrpAttr.enPixelFormat)) {
		if ((pstVideoFrame->stVFrame.u32Width - pstVideoFrame->stVFrame.s16OffsetLeft -
		     pstVideoFrame->stVFrame.s16OffsetRight) & 0x01) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) YUV420 can't accept odd frame valid width\n", VpssGrp);
			CVI_TRACE_VPSS(CVI_DBG_ERR, "u32Width(%d) s16OffsetLeft(%d) s16OffsetRight(%d)\n",
				pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.s16OffsetLeft,
				pstVideoFrame->stVFrame.s16OffsetRight);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
		if ((pstVideoFrame->stVFrame.u32Height - pstVideoFrame->stVFrame.s16OffsetTop -
		     pstVideoFrame->stVFrame.s16OffsetBottom) & 0x01) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) YUV420 can't accept odd frame valid height\n", VpssGrp);
			CVI_TRACE_VPSS(CVI_DBG_ERR, "u32Height(%d) s16OffsetTop(%d) s16OffsetBottom(%d)\n",
				pstVideoFrame->stVFrame.u32Height, pstVideoFrame->stVFrame.s16OffsetTop,
				pstVideoFrame->stVFrame.s16OffsetBottom);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
	}
	if (IS_FMT_YUV422(vpssCtx[VpssGrp]->stGrpAttr.enPixelFormat)) {
		if ((pstVideoFrame->stVFrame.u32Width - pstVideoFrame->stVFrame.s16OffsetLeft -
		     pstVideoFrame->stVFrame.s16OffsetRight) & 0x01) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) YUV422 can't accept odd frame valid width\n", VpssGrp);
			CVI_TRACE_VPSS(CVI_DBG_ERR, "u32Width(%d) s16OffsetLeft(%d) s16OffsetRight(%d)\n",
				pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.s16OffsetLeft,
				pstVideoFrame->stVFrame.s16OffsetRight);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	if (pstVideoFrame->u32PoolId == EXT_POOL){
		vpssCtx[VpssGrp]->hw_cfg.stGrpCfg.vpss_v_priority = true;
		blk = vb_create_block_vpss(&vb_bm[VpssGrp][EXT_POOL_IN], pstVideoFrame->stVFrame.u64PhyAddr[0], NULL, VB_EXTERNAL_POOLID, CVI_TRUE);
		if (blk == VB_INVALID_HANDLE) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) no space for malloc.\n", VpssGrp);
			return CVI_ERR_VPSS_NOMEM;
		}
	} else {
		blk = vb_phys_addr2handle(pstVideoFrame->stVFrame.u64PhyAddr[0]);
		if (blk == VB_INVALID_HANDLE) {
			blk = vb_create_block(pstVideoFrame->stVFrame.u64PhyAddr[0], NULL, VB_EXTERNAL_POOLID, CVI_TRUE);
			if (blk == VB_INVALID_HANDLE) {
				CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) no space for malloc.\n", VpssGrp);
				return CVI_ERR_VPSS_NOMEM;
			}
		}
	}

	if (base_fill_videoframe2buffer(chn, pstVideoFrame, &((struct vb_s *)blk)->buf) != CVI_SUCCESS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Invalid parameter\n", VpssGrp);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	if (vpss_grp_qbuf(chn, blk) != CVI_SUCCESS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "vpss_grp_qbuf(%d) failed\n", VpssGrp);
		return CVI_ERR_VPSS_BUSY;
	}

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d), phy-address(0x%llx)\n",
			VpssGrp, pstVideoFrame->stVFrame.u64PhyAddr[0]);

	return CVI_SUCCESS;
}

s32 vpss_send_chn_frame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn
	, const VIDEO_FRAME_INFO_S *pstVideoFrame, s32 s32MilliSec)
{
	s32 ret;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = VpssChn};
	VB_BLK blk;
	struct vb_s *vb;
	struct vb_jobs_t *jobs;

	ret = mod_check_null_ptr(CVI_ID_VPSS, pstVideoFrame);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!vpssCtx[VpssGrp]->isStarted) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) not yet started.\n", VpssGrp);
		return CVI_ERR_VPSS_NOTREADY;
	}
	if (!vpssCtx[VpssGrp]->stChnCfgs[VpssChn].isEnabled) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) not yet enabled.\n", VpssGrp, VpssChn);
		return CVI_ERR_VPSS_NOTREADY;
	}
	if (vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.enPixelFormat != pstVideoFrame->stVFrame.enPixelFormat) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) PixelFormat(%d) mismatch.\n"
			, VpssGrp, VpssChn, pstVideoFrame->stVFrame.enPixelFormat);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}
	if ((vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.u32Width != pstVideoFrame->stVFrame.u32Width)
	 || (vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.u32Height != pstVideoFrame->stVFrame.u32Height)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) Size(%d * %d) mismatch.\n"
			, VpssGrp, VpssChn, pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.u32Height);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	UNUSED(s32MilliSec);

	if (pstVideoFrame->u32PoolId == EXT_POOL){
		blk = vb_create_block_vpss(&vb_bm[VpssGrp][EXT_POOL_OUT], pstVideoFrame->stVFrame.u64PhyAddr[0], NULL, VB_EXTERNAL_POOLID, CVI_TRUE);
		if (blk == VB_INVALID_HANDLE) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) no space for malloc.\n", VpssGrp);
			return CVI_ERR_VPSS_NOMEM;
		}
	} else {
		blk = vb_phys_addr2handle(pstVideoFrame->stVFrame.u64PhyAddr[0]);
		if (blk == VB_INVALID_HANDLE) {
			blk = vb_create_block(pstVideoFrame->stVFrame.u64PhyAddr[0], NULL, VB_EXTERNAL_POOLID, CVI_TRUE);
			if (blk == VB_INVALID_HANDLE) {
				CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) no space for malloc.\n", VpssGrp);
				return CVI_ERR_VPSS_NOMEM;
			}
		}
	}

	if (base_fill_videoframe2buffer(chn, pstVideoFrame, &((struct vb_s *)blk)->buf) != CVI_SUCCESS) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) Invalid parameter\n", VpssGrp, VpssChn);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	jobs = &stVpssVbjobs[VpssGrp].outs[VpssChn];
	if (!jobs) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) get job failed\n",
				VpssGrp, VpssChn);
		return CVI_FAILURE;
	}

	vb = (struct vb_s *)blk;
	mutex_lock(&jobs->lock);
	if (FIFO_FULL(&jobs->waitq)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) waitq is full\n", VpssGrp, VpssChn);
		mutex_unlock(&jobs->lock);
		return CVI_FAILURE;
	}
	FIFO_PUSH(&jobs->waitq, vb);
	atomic_fetch_add(1, &vb->usr_cnt);
	mutex_unlock(&jobs->lock);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);
	return ret;
}

s32 vpss_get_chn_frame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VIDEO_FRAME_INFO_S *pstFrameInfo,
			   s32 s32MilliSec)
{
	s32 ret, i;
	VB_BLK blk = VB_INVALID_HANDLE;
	struct vb_s *vb;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = VpssChn};

	ret = mod_check_null_ptr(CVI_ID_VPSS, pstFrameInfo);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!vpssCtx[VpssGrp]->isStarted) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) not yet started.\n", VpssGrp);
		return CVI_ERR_VPSS_NOTREADY;
	}
	if (!vpssCtx[VpssGrp]->stChnCfgs[VpssChn].isEnabled) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) not yet enabled.\n", VpssGrp, VpssChn);
		return CVI_ERR_VPSS_NOTREADY;
	}

	memset(pstFrameInfo, 0, sizeof(*pstFrameInfo));
	ret = base_get_chn_buffer(chn, &stVpssVbjobs[VpssGrp].outs[VpssChn], &blk, s32MilliSec);
	if (ret != 0 || blk == VB_INVALID_HANDLE) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) get chn frame null, s32MilliSec=%d, ret=%d\n",
				VpssGrp, VpssChn, s32MilliSec, ret);
		return CVI_ERR_VPSS_BUF_EMPTY;
	}

	vb = (struct vb_s *)blk;
	if (!vb->buf.phy_addr[0] || !vb->buf.size.u32Width) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "buf already released\n");
		return CVI_ERR_VPSS_BUF_EMPTY;
	}

	pstFrameInfo->stVFrame.enPixelFormat = vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.enPixelFormat;
	pstFrameInfo->stVFrame.u32Width = vb->buf.size.u32Width;
	pstFrameInfo->stVFrame.u32Height = vb->buf.size.u32Height;
	pstFrameInfo->stVFrame.u32TimeRef = vb->buf.frm_num;
	pstFrameInfo->stVFrame.u64PTS = vb->buf.u64PTS;
	pstFrameInfo->stVFrame.u32FrameFlag = vb->buf.u32FrameFlag;
	for (i = 0; i < 3; ++i) {
		pstFrameInfo->stVFrame.u64PhyAddr[i] = vb->buf.phy_addr[i];
		pstFrameInfo->stVFrame.u32Length[i] = vb->buf.length[i];
		pstFrameInfo->stVFrame.u32Stride[i] = vb->buf.stride[i];
	}

	pstFrameInfo->stVFrame.s16OffsetTop = vb->buf.s16OffsetTop;
	pstFrameInfo->stVFrame.s16OffsetBottom = vb->buf.s16OffsetBottom;
	pstFrameInfo->stVFrame.s16OffsetLeft = vb->buf.s16OffsetLeft;
	pstFrameInfo->stVFrame.s16OffsetRight = vb->buf.s16OffsetRight;
	pstFrameInfo->stVFrame.pPrivateData = vb;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) end to set pstFrameInfo width:%d height:%d buf:0x%llx\n"
			, VpssGrp, VpssChn, pstFrameInfo->stVFrame.u32Width, pstFrameInfo->stVFrame.u32Height,
			pstFrameInfo->stVFrame.u64PhyAddr[0]);
	return CVI_SUCCESS;
}

s32 vpss_bm_send_frame(bm_vpss_cfg *vpss_cfg){

	VPSS_GRP VpssGrp = VPSS_INVALID_GRP;
	s32 ret = CVI_SUCCESS, i = 0;

	if(!is_bm_scene) is_bm_scene = true;

	VpssGrp = vpss_get_available_grp();
	if (VPSS_INVALID_GRP == VpssGrp) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) not ready.\n", VpssGrp);
		return CVI_ERR_VPSS_NOTREADY;
	}

	ret = vpss_create_grp(VpssGrp, &vpss_cfg->grp_attr.stGrpAttr);
	if (ret != CVI_SUCCESS)
		goto fail;

	ret = vpss_reset_grp(VpssGrp);
	if (ret != CVI_SUCCESS)
		goto fail;

	ret = vpss_set_chn_attr(VpssGrp, 0, &vpss_cfg->chn_attr.stChnAttr);
	if (ret != CVI_SUCCESS)
		goto fail;

	ret = vpss_enable_chn(VpssGrp, 0);
	if (ret != CVI_SUCCESS)
		goto fail;

	ret = vpss_start_grp(VpssGrp);
	if (ret != CVI_SUCCESS)
		goto fail;

	ret = vpss_set_chn_scale_coef_level(VpssGrp, 0, vpss_cfg->chn_coef_level_cfg.enCoef);
	if (ret != CVI_SUCCESS)
		goto fail;

	if(vpss_cfg->grp_csc_cfg.enable){
		vpss_cfg->grp_csc_cfg.VpssGrp = VpssGrp;
		ret = vpss_set_grp_csc(&vpss_cfg->grp_csc_cfg);
		if (ret != CVI_SUCCESS)
			goto fail;
	}

	if(vpss_cfg->chn_csc_cfg.enable){
		vpss_cfg->chn_csc_cfg.VpssGrp = VpssGrp;
		vpss_cfg->chn_csc_cfg.VpssChn = 0;
		ret = vpss_set_chn_csc(&vpss_cfg->chn_csc_cfg);
		if (ret != CVI_SUCCESS)
			goto fail;
	}

	if(vpss_cfg->grp_crop_cfg.stCropInfo.bEnable){
		ret = vpss_set_grp_crop(VpssGrp, &vpss_cfg->grp_crop_cfg.stCropInfo);
		if (ret != CVI_SUCCESS)
			goto fail;
	}

	if(vpss_cfg->chn_crop_cfg.stCropInfo.bEnable){
		ret = vpss_set_chn_crop(VpssGrp, 0, &vpss_cfg->chn_crop_cfg.stCropInfo);
		if (ret != CVI_SUCCESS)
			goto fail;
	}

	if(vpss_cfg->chn_convert_cfg.stConvert.bEnable){
		ret = vpss_set_chn_convert(VpssGrp, 0, &vpss_cfg->chn_convert_cfg.stConvert);
		if (ret != CVI_SUCCESS)
			goto fail;
	}

	if(vpss_cfg->chn_draw_rect_cfg.stDrawRect.astRect[0].bEnable){
		ret = vpss_set_chn_draw_rect(VpssGrp, 0, &vpss_cfg->chn_draw_rect_cfg.stDrawRect);
		if (ret != CVI_SUCCESS)
			goto fail;
	}

	ret = vpss_set_chn_align(VpssGrp, 0, 1);
	if (ret != CVI_SUCCESS)
		goto fail;

	if(vpss_cfg->coverex_cfg.rgn_coverex_cfg.rgn_coverex_param[0].enable){
		ret = vpss_set_rgn_coverex_cfg(VpssGrp, 0, &vpss_cfg->coverex_cfg.rgn_coverex_cfg);
		if (ret != CVI_SUCCESS)
			goto fail;
	}

	for(i = 0; i < RGN_MAX_LAYER_VPSS; i++)
		if(vpss_cfg->rgn_cfg[i].num_of_rgn > 0){
			ret = vpss_set_rgn_cfg(VpssGrp, 0, i, &vpss_cfg->rgn_cfg[i]);
			if (ret != CVI_SUCCESS)
				goto fail;
		}

	ret = vpss_send_chn_frame(VpssGrp, 0, &vpss_cfg->chn_frm_cfg.stVideoFrame, vpss_cfg->chn_frm_cfg.s32MilliSec);
	if (ret != CVI_SUCCESS)
		goto fail;

	ret = vpss_send_frame(VpssGrp, &vpss_cfg->snd_frm_cfg.stVideoFrame, vpss_cfg->snd_frm_cfg.s32MilliSec);
	if (ret != CVI_SUCCESS)
		goto fail;

	ret = vpss_get_chn_frame(VpssGrp, 0, &vpss_cfg->chn_frm_cfg.stVideoFrame, vpss_cfg->chn_frm_cfg.s32MilliSec);

fail:
	vpss_disable_chn(VpssGrp, 0);
	vpss_stop_grp(VpssGrp);
	vpss_destroy_grp(VpssGrp);

	return ret;
}

s32 vpss_release_chn_frame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	VB_BLK blk;
	s32 ret;

	ret = mod_check_null_ptr(CVI_ID_VPSS, pstVideoFrame);
	if (ret != CVI_SUCCESS)
		return ret;

	blk = vb_phys_addr2handle(pstVideoFrame->stVFrame.u64PhyAddr[0]);
	if (blk == VB_INVALID_HANDLE) {
		if (pstVideoFrame->stVFrame.pPrivateData == 0) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) phy-address(0x%llx) invalid to locate.\n"
				      , VpssGrp, VpssChn, (unsigned long long)pstVideoFrame->stVFrame.u64PhyAddr[0]);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
		blk = (VB_BLK)pstVideoFrame->stVFrame.pPrivateData;
	}

	if (vb_release_block(blk) != CVI_SUCCESS)
		return CVI_FAILURE;

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) buf:0x%llx\n",
			VpssGrp, VpssChn, pstVideoFrame->stVFrame.u64PhyAddr[0]);
	return CVI_SUCCESS;
}

s32 vpss_set_chn_rotation(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, ROTATION_E enRotation)
{
	s32 ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_gdc_fmt(VpssGrp, VpssChn, vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.enPixelFormat);
	if (ret != CVI_SUCCESS)
		return ret;

	return _vpss_update_rotation_mesh(VpssGrp, VpssChn, enRotation);
}

s32 vpss_get_chn_rotation(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, ROTATION_E *penRotation)
{
	s32 ret;

	ret = mod_check_null_ptr(CVI_ID_VPSS, penRotation);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	*penRotation = vpssCtx[VpssGrp]->stChnCfgs[VpssChn].enRotation;
	return CVI_SUCCESS;
}

s32 vpss_set_chn_align(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, u32 u32Align)
{
	s32 ret;
	VB_CAL_CONFIG_S stVbCalConfig;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (u32Align > MAX_ALIGN) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) alignment(%d) exceeding the maximum value %d\n",
			VpssGrp, VpssChn, u32Align, MAX_ALIGN);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	COMMON_GetPicBufferConfig(vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.u32Width,
		vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.u32Height,
		vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.enPixelFormat,
		DATA_BITWIDTH_8, COMPRESS_MODE_NONE, u32Align, &stVbCalConfig);

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].blk_size = stVbCalConfig.u32VBSize;
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].align = u32Align;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) u32Align:%d\n", VpssGrp, VpssChn, u32Align);
	return CVI_SUCCESS;
}

s32 vpss_get_chn_align(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, u32 *pu32Align)
{
	s32 ret;

	ret = mod_check_null_ptr(CVI_ID_VPSS, pu32Align);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	*pu32Align = vpssCtx[VpssGrp]->stChnCfgs[VpssChn].align;
	return CVI_SUCCESS;
}

s32 vpss_set_chn_scale_coef_level(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_SCALE_COEF_E enCoef)
{
	s32 ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (enCoef >= VPSS_SCALE_COEF_MAX) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) undefined scale_coef type(%d)\n"
			, VpssGrp, VpssChn, enCoef);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].enCoef = enCoef;
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);
	return CVI_SUCCESS;
}

s32 vpss_get_chn_scale_coef_level(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_SCALE_COEF_E *penCoef)
{
	s32 ret;

	ret = mod_check_null_ptr(CVI_ID_VPSS, penCoef);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	*penCoef = vpssCtx[VpssGrp]->stChnCfgs[VpssChn].enCoef;
	return CVI_SUCCESS;
}

s32 vpss_set_chn_draw_rect(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_DRAW_RECT_S *pstDrawRect)
{
	s32 i, ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	for (i = 0; i < VPSS_RECT_NUM; i++) {
		if (!pstDrawRect->astRect[i].bEnable)
			continue;
		if (pstDrawRect->astRect[i].stRect.u32Width < (2 * pstDrawRect->astRect[i].u16Thick)) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d), Width less than 2 times thickness.\n",
				VpssGrp, VpssChn);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
		if (pstDrawRect->astRect[i].stRect.u32Height < (2 * pstDrawRect->astRect[i].u16Thick)) {
			CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d), Height less than 2 times thickness.\n",
				VpssGrp, VpssChn);
			return CVI_ERR_VPSS_ILLEGAL_PARAM;
		}
	}

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stDrawRect = *pstDrawRect;
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);
	return CVI_SUCCESS;
}

s32 vpss_get_chn_draw_rect(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_DRAW_RECT_S *pstDrawRect)
{
	s32 ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	*pstDrawRect = vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stDrawRect;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	return CVI_SUCCESS;
}

s32 vpss_set_chn_convert(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, const VPSS_CONVERT_S *pstConvert)
{
	s32 ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stConvert = *pstConvert;
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);
	return CVI_SUCCESS;
}

s32 vpss_get_chn_convert(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_CONVERT_S *pstConvert)
{
	s32 ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	*pstConvert = vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stConvert;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	return CVI_SUCCESS;
}

/* CVI_VPSS_SetChnYRatio: Modify the y ratio of chn output. Only work for yuv format.
 *
 * @param VpssGrp: The Vpss Grp to work.
 * @param VpssChn: The Vpss Chn to work.
 * @param YRatio: Output's Y will be sacled by this ratio.
 * @return: CVI_SUCCESS if OK.
 */
s32 vpss_set_chn_yratio(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, u32 YRatio)
{
	s32 ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (!IS_FMT_YUV(vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.enPixelFormat)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) isn't YUV format. Can't apply this setting.\n"
			, VpssGrp, VpssChn);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	if (vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.stNormalize.bEnable) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) Y-ratio adjustment can't work with normalize.\n"
			, VpssGrp, VpssChn);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	if (YRatio > YRATIO_SCALE) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Grp(%d) Chn(%d) YRatio(%d) out of range(0-%d).\n"
			, VpssGrp, VpssChn, YRatio, YRATIO_SCALE);
		return CVI_ERR_VPSS_ILLEGAL_PARAM;
	}

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].YRatio = YRatio;
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].is_cfg_changed = CVI_TRUE;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);
	return CVI_SUCCESS;
}

s32 vpss_get_chn_yratio(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, u32 *pYRatio)
{
	s32 ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	*pYRatio = vpssCtx[VpssGrp]->stChnCfgs[VpssChn].YRatio;
	return CVI_SUCCESS;
}

s32 vpss_set_chn_ldc_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn,
				const VPSS_LDC_ATTR_S *pstLDCAttr, u64 mesh_addr)
{
	s32 ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (pstLDCAttr->stAttr.bEnHWLDC)
		ret = check_vpss_gdc_fmt(VpssGrp, VpssChn, vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.enPixelFormat);
	else
		ret = check_vpss_dwa_fmt(VpssGrp, VpssChn, vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.enPixelFormat);
	if (ret != CVI_SUCCESS)
		return ret;

	return _vpss_update_ldc_mesh(VpssGrp, VpssChn, pstLDCAttr, mesh_addr);
}

s32 vpss_get_chn_ldc_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VPSS_LDC_ATTR_S *pstLDCAttr)
{
	s32 ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	memcpy(pstLDCAttr, &vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stLDCAttr, sizeof(*pstLDCAttr));

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "bEnable=%d, apect=%d, xyratio=%d, xoffset=%d, yoffset=%d, ratio=%d\n",
			pstLDCAttr->bEnable, pstLDCAttr->stAttr.bAspect,
			pstLDCAttr->stAttr.s32XYRatio, pstLDCAttr->stAttr.s32CenterXOffset,
			pstLDCAttr->stAttr.s32CenterYOffset, pstLDCAttr->stAttr.s32DistortionRatio);

	return CVI_SUCCESS;
}

s32 vpss_set_chn_fisheye_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, ROTATION_E enRotation,
				const FISHEYE_ATTR_S *pstFishEyeAttr, u64 mesh_addr)
{
	s32 ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (pstFishEyeAttr->bEnable)
		ret = check_vpss_dwa_fmt(VpssGrp, VpssChn, vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stChnAttr.enPixelFormat);
	if (ret != CVI_SUCCESS)
		return ret;

	return _vpss_update_fisheye_mesh(VpssGrp, VpssChn, pstFishEyeAttr, enRotation, mesh_addr);
}

s32 vpss_get_chn_fisheye_attr(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, FISHEYE_ATTR_S *pstFishEyeAttr)
{
	s32 ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	memcpy(pstFishEyeAttr, &vpssCtx[VpssGrp]->stChnCfgs[VpssChn].stFishEyeAttr, sizeof(*pstFishEyeAttr));

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "bEnable=%d, bBgColor=%d, u32BgColor=%d, s32HorOffset=%d, s32VerOffset=%d, u32TrapezoidCoef=%d, s32FanStrength=%d,\
			enMountMode=%d, enUseMode=%d, u32RegionNum=%d\n",
			pstFishEyeAttr->bEnable, pstFishEyeAttr->bBgColor,
			pstFishEyeAttr->u32BgColor, pstFishEyeAttr->s32HorOffset,
			pstFishEyeAttr->s32VerOffset, pstFishEyeAttr->u32TrapezoidCoef,
			pstFishEyeAttr->s32FanStrength, pstFishEyeAttr->enMountMode,
			pstFishEyeAttr->enUseMode, pstFishEyeAttr->u32RegionNum);

	return CVI_SUCCESS;
}

s32 vpss_attach_vb_pool(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VB_POOL hVbPool)
{
	s32 ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].VbPool = hVbPool;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d) attach vb pool(%d)\n",
		VpssGrp, VpssChn, hVbPool);
	return CVI_SUCCESS;
}

s32 vpss_detach_vb_pool(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
	s32 ret;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	mutex_lock(&vpssCtx[VpssGrp]->lock);
	vpssCtx[VpssGrp]->stChnCfgs[VpssChn].VbPool = VB_INVALID_POOLID;
	mutex_unlock(&vpssCtx[VpssGrp]->lock);

	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);
	return CVI_SUCCESS;
}

s32 vpss_trigger_snap_frame(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, u32 frame_cnt)
{
	s32 ret;
	MMF_BIND_DEST_S stBindDest;
	MMF_CHN_S chn = {.enModId = CVI_ID_VPSS, .s32DevId = VpssGrp, .s32ChnId = VpssChn};
	struct base_exe_m_cb exe_cb;
	struct venc_snap_frm_info sanp_info;

	ret = check_vpss_grp_valid(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_grp_created(VpssGrp);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = check_vpss_chn_valid(VpssGrp, VpssChn);
	if (ret != CVI_SUCCESS)
		return ret;

	if (bind_get_dst(&chn, &stBindDest) != CVI_SUCCESS) {
		CVI_TRACE_VPSS(CVI_DBG_WARN, "sys_get_bindbysrc fails\n");
		return CVI_ERR_VPSS_NOT_PERM;
	}
	if (stBindDest.astMmfChn[0].enModId != CVI_ID_VENC) {
		CVI_TRACE_VPSS(CVI_DBG_INFO, "next Mod(%d) is not vcodec\n",
				stBindDest.astMmfChn[0].enModId);
		return CVI_ERR_VPSS_NOT_PERM;
	}
	CVI_TRACE_VPSS(CVI_DBG_DEBUG, "Grp(%d) Chn(%d)\n", VpssGrp, VpssChn);

	sanp_info.vpss_grp = VpssGrp;
	sanp_info.vpss_chn = VpssChn;
	sanp_info.skip_frm_cnt = frame_cnt;

	exe_cb.callee = E_MODULE_VCODEC;
	exe_cb.caller = E_MODULE_VPSS;
	exe_cb.cmd_id = VCODEC_CB_SNAP_JPG_FRM;
	exe_cb.data   = &sanp_info;
	ret = base_exe_module_cb(&exe_cb);

	return ret;
}

void stitch_wakeup(void *data)
{
	struct vpss_stitch_data *pstStitchData = (struct vpss_stitch_data *)data;

	pstStitchData->u8Flag = 1;
	wake_up(&pstStitchData->wait);
}

s32 vpss_stitch(u32 u32ChnNum, VPSS_STITCH_CHN_ATTR_S *pstInput,
			VPSS_STITCH_OUTPUT_ATTR_S *pstOutput, VIDEO_FRAME_INFO_S *pstVideoFrame)
{
	s32 ret = CVI_SUCCESS;
	struct vpss_stitch_cfg stCfg;
	struct stitch_dst_cfg dst_cfg;
	struct stitch_chn_cfg *chn_cfg = NULL;
	unsigned long timeout = msecs_to_jiffies(EOF_WAIT_TIMEOUT_MS);
	u8 i;
	struct vpss_stitch_data *pstStitchData = NULL;
	VB_CAL_CONFIG_S stVbCalConfig;
	VB_BLK blk = VB_INVALID_HANDLE;
	struct vb_s *vb;
	SIZE_S stOutSize = {pstOutput->u32Width, pstOutput->u32Height};

	chn_cfg = (struct stitch_chn_cfg *)vzalloc(sizeof(struct stitch_chn_cfg) * u32ChnNum);
	if (!chn_cfg) {
		return CVI_ERR_VPSS_NOMEM;
	}

	for (i = 0; i < u32ChnNum; i++) {
		chn_cfg[i].pixelformat = pstInput[i].stVideoFrame.stVFrame.enPixelFormat;
		chn_cfg[i].bytesperline[0] = pstInput[i].stVideoFrame.stVFrame.u32Stride[0];
		chn_cfg[i].bytesperline[1] = pstInput[i].stVideoFrame.stVFrame.u32Stride[1];
		chn_cfg[i].addr[0] = pstInput[i].stVideoFrame.stVFrame.u64PhyAddr[0];
		chn_cfg[i].addr[1] = pstInput[i].stVideoFrame.stVFrame.u64PhyAddr[1];
		chn_cfg[i].addr[2] = pstInput[i].stVideoFrame.stVFrame.u64PhyAddr[2];
		chn_cfg[i].src_size.width = pstInput[i].stVideoFrame.stVFrame.u32Width;
		chn_cfg[i].src_size.height = pstInput[i].stVideoFrame.stVFrame.u32Height;
		chn_cfg[i].rect_crop.left = 0;
		chn_cfg[i].rect_crop.top = 0;
		chn_cfg[i].rect_crop.width = pstInput[i].stVideoFrame.stVFrame.u32Width;
		chn_cfg[i].rect_crop.height = pstInput[i].stVideoFrame.stVFrame.u32Height;

		chn_cfg[i].window.rect_out.left = pstInput[i].stDstRect.s32X;
		chn_cfg[i].window.rect_out.top = pstInput[i].stDstRect.s32Y;
		chn_cfg[i].window.rect_out.width = pstInput[i].stDstRect.u32Width;
		chn_cfg[i].window.rect_out.height = pstInput[i].stDstRect.u32Height;
		chn_cfg[i].window.rect_in.left = pstInput[i].stDstRect.s32X;
		chn_cfg[i].window.rect_in.top = pstInput[i].stDstRect.s32Y;
		chn_cfg[i].window.rect_in.width = pstInput[i].stDstRect.u32Width;
		chn_cfg[i].window.rect_in.height = pstInput[i].stDstRect.u32Height;
	}

	COMMON_GetPicBufferConfig(pstOutput->u32Width, pstOutput->u32Height,
		pstOutput->enPixelformat, DATA_BITWIDTH_8
		, COMPRESS_MODE_NONE, DEFAULT_ALIGN, &stVbCalConfig);

	blk = vb_get_block_with_id(VB_INVALID_POOLID, stVbCalConfig.u32VBSize, CVI_ID_VPSS);
	if (blk == VB_INVALID_HANDLE) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "Can't acquire VB BLK for vpss stitch.\n");
		ret = CVI_ERR_VPSS_NOBUF;
		goto EXIT;
	}

	vb = (struct vb_s *)blk;
	base_get_frame_info(pstOutput->enPixelformat
			   , stOutSize
			   , &vb->buf
			   , vb_handle2phys_addr(blk)
			   , DEFAULT_ALIGN);

	dst_cfg.bytesperline[0] = vb->buf.stride[0];
	dst_cfg.bytesperline[1] = vb->buf.stride[1];
	dst_cfg.addr[0] = vb->buf.phy_addr[0];
	dst_cfg.addr[1] = vb->buf.phy_addr[1];
	dst_cfg.addr[2] = vb->buf.phy_addr[2];
	dst_cfg.pixelformat = pstOutput->enPixelformat;
	dst_cfg.color[0] = (pstOutput->u32Color & 0xFF0000) >> 16;
	dst_cfg.color[1] = (pstOutput->u32Color & 0xFF00) >> 8;
	dst_cfg.color[2] = pstOutput->u32Color & 0xFF;
	dst_cfg.dst_size.width = pstOutput->u32Width;
	dst_cfg.dst_size.height = pstOutput->u32Height;

	pstStitchData = (struct vpss_stitch_data *)vmalloc(sizeof(struct vpss_stitch_data));
	if (!pstStitchData) {
		vb_release_block(blk);
		goto EXIT;
	}

	init_waitqueue_head(&pstStitchData->wait);
	pstStitchData->u8Flag = 0;

	stCfg.num = u32ChnNum;
	stCfg.chn_cfg = chn_cfg;
	stCfg.dst_cfg = dst_cfg;
	stCfg.data = (void *)pstStitchData;
	stCfg.pfnJobCB = stitch_wakeup;

	ret = cvi_vpss_hal_stitch_schedule(&stCfg);
	if (ret) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "cvi_vpss_hal_stitch_schedule fail!\n");
		vb_release_block(blk);
		goto EXIT;
	}

	ret = wait_event_timeout(pstStitchData->wait, pstStitchData->u8Flag, timeout);
	pstStitchData->u8Flag = 0;

	if (ret < 0) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "-ERESTARTSYS!\n");
		vb_release_block(blk);
		ret = CVI_FAILURE;
	} else if (ret == 0) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "timeout!\n");
		vb_release_block(blk);
		ret = CVI_FAILURE;
	} else {
		memset(pstVideoFrame, 0, sizeof(*pstVideoFrame));
		pstVideoFrame->stVFrame.enPixelFormat = pstOutput->enPixelformat;
		pstVideoFrame->stVFrame.u32Width = vb->buf.size.u32Width;
		pstVideoFrame->stVFrame.u32Height = vb->buf.size.u32Height;
		for (i = 0; i < 3; ++i) {
			pstVideoFrame->stVFrame.u64PhyAddr[i] = vb->buf.phy_addr[i];
			pstVideoFrame->stVFrame.u32Length[i] = vb->buf.length[i];
			pstVideoFrame->stVFrame.u32Stride[i] = vb->buf.stride[i];
		}
		ret = CVI_SUCCESS;
	}

EXIT:
	if (chn_cfg)
		vfree(chn_cfg);

	if (pstStitchData)
		vfree(pstStitchData);

	return ret;
}

s32 vpss_set_vivpss_mode(const VI_VPSS_MODE_S *pstVIVPSSMode)
{
	memcpy(&stVIVPSSMode, pstVIVPSSMode, sizeof(stVIVPSSMode));

	return CVI_SUCCESS;
}

void vpss_set_mlv_info(u8 snr_num, struct mlv_i_s *p_m_lv_i)
{
	if (snr_num >= ARRAY_SIZE(g_mlv_i)) {
		CVI_TRACE_VPSS(CVI_DBG_INFO, "snr_num(%d) out of range\n", snr_num);
		return;
	}

	g_mlv_i[snr_num].mlv_i_level = p_m_lv_i->mlv_i_level;
	memcpy(g_mlv_i[snr_num].mlv_i_table, p_m_lv_i->mlv_i_table, sizeof(g_mlv_i[snr_num].mlv_i_table));
}

void vpss_get_mlv_info(u8 snr_num, struct mlv_i_s *p_m_lv_i)
{
	if (snr_num >= ARRAY_SIZE(g_mlv_i)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "snr_num(%d) out of range\n", snr_num);
		return;
	}
	p_m_lv_i->mlv_i_level = g_mlv_i[snr_num].mlv_i_level;
	memcpy(p_m_lv_i->mlv_i_table, g_mlv_i[snr_num].mlv_i_table, sizeof(g_mlv_i[snr_num].mlv_i_table));
}

void vpss_mode_init(void)
{
	u8 i, j;
	PROC_AMP_CTRL_S ctrl;

	mutex_lock(&g_VpssLock);
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		for (j = PROC_AMP_BRIGHTNESS; j < PROC_AMP_MAX; ++j) {
			vpss_get_proc_amp_ctrl(j, &ctrl);
			vpssExtCtx[i].proc_amp[j] = ctrl.default_value;
		}
		s_VpssGrpUsed[i] = CVI_FALSE;
	}

	//init_timer(&timer_proc);
	//timer_proc.function = _update_vpss_chn_real_frame_rate;
	//timer_proc.expires = jiffies + msecs_to_jiffies(1000);
	//timer_proc.data = 0;
	add_timer(&timer_proc);
	mod_timer(&timer_proc, jiffies + msecs_to_jiffies(1000));
	mutex_unlock(&g_VpssLock);
}

void vpss_mode_deinit(void)
{
	mutex_lock(&g_VpssLock);
	del_timer_sync(&timer_proc);
	mutex_unlock(&g_VpssLock);
}

void register_timer_fun(vpss_timer_cb cb, void *data)
{
	s_core_cb = cb;
	s_core_data = data;
}

void vpss_init(void)
{
	int ret;
	struct sched_param tsk;

	base_register_recv_cb(CVI_ID_VPSS, vpss_grp_qbuf);


	mutex_init(&g_VpssLock);
	init_waitqueue_head(&vpss_hdl_ctx.wait);
	spin_lock_init(&vpss_hdl_ctx.hdl_lock);
	vpss_hdl_ctx.GrpMask = 0;
	vpss_hdl_ctx.events = 0;

	// Same as sched_set_fifo in linux 5.x
	tsk.sched_priority = MAX_USER_RT_PRIO - 10;

	vpss_hdl_ctx.thread = kthread_run(vpss_event_handler, &vpss_hdl_ctx,
		"cvitask_vpss_hdl");
	if (IS_ERR(vpss_hdl_ctx.thread)) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "failed to create vpss kthread\n");
	}

	ret = sched_setscheduler(vpss_hdl_ctx.thread, SCHED_FIFO, &tsk);
	if (ret)
		CVI_TRACE_VPSS(CVI_DBG_WARN, "vpss thread priority update failed: %d\n", ret);

	vpss_workqueue = alloc_workqueue("vpss_workqueue", WQ_HIGHPRI, 0);
	//workqueue = create_workqueue("vpss_workqueue");
	// workqueue = create_singlethread_workqueue("vpss_workqueue");
	if (!vpss_workqueue)
		CVI_TRACE_VPSS(CVI_DBG_ERR, "vpss create_workqueue failed.\n");
}

void vpss_deinit(void)
{
	int ret;

	if (!vpss_hdl_ctx.thread) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "vpss thread not initialized yet\n");
		return;
	}
	base_unregister_recv_cb(CVI_ID_VPSS);

	ret = kthread_stop(vpss_hdl_ctx.thread);
	if (ret)
		CVI_TRACE_VPSS(CVI_DBG_ERR, "fail to stop vpss thread, err=%d\n", ret);

	vpss_hdl_ctx.thread = NULL;
	destroy_workqueue(vpss_workqueue);
	mutex_destroy(&g_VpssLock);
}

void vpss_release_grp(void)
{
	VPSS_GRP grp;
	VPSS_CHN chn;

	for (grp = 0; grp < VPSS_MAX_GRP_NUM; grp++) {
		if (vpssCtx[grp] && vpssCtx[grp]->isCreated) {
			for (chn = 0; chn < VPSS_MAX_CHN_NUM; chn++) {
				if (vpssCtx[grp]->stChnCfgs[chn].isEnabled) {
					vpss_disable_chn(grp, chn);
				}
			}

			vpss_stop_grp(grp);
			vpss_destroy_grp(grp);
		}
	}
}