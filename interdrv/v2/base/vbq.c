#include <linux/types.h>
#include <linux/mm.h>
#include <linux/cvi_buffer.h>
#include <linux/slab.h>

#include "vbq.h"
#include "vb.h"
#include "bind.h"
#include "base_common.h"


static VBQ_RECV_CB base_recv_cb[CVI_ID_BUTT];
//int32_t (*base_qbuf_cb[CVI_ID_BUTT])(struct cvi_buffer *buf, MMF_CHN_S chn) = {0};
//int32_t (*base_dqbuf_cb[CVI_ID_BUTT])(struct cvi_buffer *buf, MMF_CHN_S chn) = {0};

void base_register_recv_cb(MOD_ID_E enModId, VBQ_RECV_CB cb)
{
	if (enModId < 0 || enModId >= CVI_ID_BUTT)
		pr_err("enModId error.\n");

	base_recv_cb[enModId] = cb;
}
EXPORT_SYMBOL_GPL(base_register_recv_cb);


void base_unregister_recv_cb(MOD_ID_E enModId)
{
	if (enModId < 0 || enModId >= CVI_ID_BUTT)
		pr_err("enModId error.\n");

	base_recv_cb[enModId] = NULL;
}
EXPORT_SYMBOL_GPL(base_unregister_recv_cb);


s32 base_fill_videoframe2buffer(MMF_CHN_S chn, const VIDEO_FRAME_INFO_S *pstVideoFrame,
	struct cvi_buffer *buf)
{
	u32 plane_size;
	VB_CAL_CONFIG_S stVbCalConfig;
	u8 i = 0;
	u32 u32Align = pstVideoFrame->stVFrame.u32Align ? pstVideoFrame->stVFrame.u32Align : DEFAULT_ALIGN;

	COMMON_GetPicBufferConfig(pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.u32Height,
		pstVideoFrame->stVFrame.enPixelFormat, DATA_BITWIDTH_8, COMPRESS_MODE_NONE,
		u32Align, &stVbCalConfig);

	buf->size.u32Width = pstVideoFrame->stVFrame.u32Width;
	buf->size.u32Height = pstVideoFrame->stVFrame.u32Height;
	buf->enPixelFormat = pstVideoFrame->stVFrame.enPixelFormat;
	buf->s16OffsetLeft = pstVideoFrame->stVFrame.s16OffsetLeft;
	buf->s16OffsetTop = pstVideoFrame->stVFrame.s16OffsetTop;
	buf->s16OffsetRight = pstVideoFrame->stVFrame.s16OffsetRight;
	buf->s16OffsetBottom = pstVideoFrame->stVFrame.s16OffsetBottom;
	buf->frm_num = pstVideoFrame->stVFrame.u32TimeRef;
	buf->u64PTS = pstVideoFrame->stVFrame.u64PTS;
	buf->enCompressMode = pstVideoFrame->stVFrame.enCompressMode;
	buf->compress_expand_addr = pstVideoFrame->stVFrame.u64ExtPhyAddr;
	memset(&buf->frame_crop, 0, sizeof(buf->frame_crop));

	for (i = 0; i < NUM_OF_PLANES; ++i) {
		if (i >= stVbCalConfig.plane_num) {
			buf->phy_addr[i] = 0;
			buf->length[i] = 0;
			buf->stride[i] = 0;
			continue;
		}

		plane_size = (i == 0) ? stVbCalConfig.u32MainYSize : stVbCalConfig.u32MainCSize;
		buf->phy_addr[i] = pstVideoFrame->stVFrame.u64PhyAddr[i];
		buf->length[i] = pstVideoFrame->stVFrame.u32Length[i];
		buf->stride[i] = pstVideoFrame->stVFrame.u32Stride[i];
		if (buf->length[i] < plane_size && (pstVideoFrame->stVFrame.enCompressMode != COMPRESS_MODE_FRAME)) {
			pr_err("Mod(%s) Dev(%d) Chn(%d) Plane[%d]\n"
				, sys_get_modname(chn.enModId), chn.s32DevId, chn.s32ChnId, i);
			pr_err(" length(%zu) less than expected(%d).\n"
				, buf->length[i], plane_size);
			return CVI_FAILURE;
		}
		if (buf->stride[i] % u32Align && (pstVideoFrame->stVFrame.enCompressMode != COMPRESS_MODE_FRAME)) {
			pr_err("Mod(%s) Dev(%d) Chn(%d) Plane[%d]\n"
				, sys_get_modname(chn.enModId), chn.s32DevId, chn.s32ChnId, i);
			pr_err(" stride(%d) not aligned(%d).\n"
				, buf->stride[i], u32Align);
			return CVI_FAILURE;
		}
		if (buf->phy_addr[i] % u32Align) {
			pr_err("Mod(%s) Dev(%d) Chn(%d) Plane[%d]\n"
				, sys_get_modname(chn.enModId), chn.s32DevId, chn.s32ChnId, i);
			pr_err(" address(%llx) not aligned(%d).\n"
				, buf->phy_addr[i], u32Align);
			return CVI_FAILURE;
		}
	}

	// [WA-01]
	if (stVbCalConfig.plane_num > 1) {
		if (((buf->phy_addr[0] & (stVbCalConfig.u16AddrAlign - 1))
			!= (buf->phy_addr[1] & (stVbCalConfig.u16AddrAlign - 1)))
		|| ((buf->phy_addr[0] & (stVbCalConfig.u16AddrAlign - 1))
			!= (buf->phy_addr[2] & (stVbCalConfig.u16AddrAlign - 1)))) {
			pr_err("Mod(%s) Dev(%d) Chn(%d)\n"
				, sys_get_modname(chn.enModId), chn.s32DevId, chn.s32ChnId);
			pr_err("plane address offset (%llx-%llx-%llx)"
				, buf->phy_addr[0], buf->phy_addr[1], buf->phy_addr[2]);
			pr_err("not aligned to %#x.\n", stVbCalConfig.u16AddrAlign);
			return CVI_FAILURE;
		}
	}
	return CVI_SUCCESS;
}
EXPORT_SYMBOL_GPL(base_fill_videoframe2buffer);

s32 base_get_chn_buffer(MMF_CHN_S chn, struct vb_jobs_t *jobs, VB_BLK *blk, s32 timeout_ms)
{
	s32 ret = CVI_FAILURE;
	struct vb_s *vb;
	struct vbq *doneq;
	struct snap_s *s;

	if (!jobs) {
		pr_err("mod(%s), jobs Null.\n", sys_get_modname(chn.enModId));
		return CVI_FAILURE;
	}

	if (!jobs->inited) {
		pr_err("mod(%s) get chn buf fail, not inited yet\n", sys_get_modname(chn.enModId));
		return CVI_FAILURE;
	}
	doneq = &jobs->doneq;

	mutex_lock(&jobs->dlock);
	if (!FIFO_EMPTY(doneq)) {
		FIFO_POP(doneq, &vb);
		atomic_long_fetch_and(~BIT(chn.enModId), &vb->mod_ids);
		atomic_long_fetch_or(BIT(CVI_ID_USER), &vb->mod_ids);
		mutex_unlock(&jobs->dlock);
		*blk = (VB_BLK)vb;
		return CVI_SUCCESS;
	}

	s = kmalloc(sizeof(*s), GFP_ATOMIC);
	if (!s) {
		mutex_unlock(&jobs->dlock);
		return CVI_FAILURE;
	}

	init_waitqueue_head(&s->cond_queue);

	s->chn = chn;
	s->blk = VB_INVALID_HANDLE;
	s->avail = CVI_FALSE;

	if (timeout_ms < 0) {
		TAILQ_INSERT_TAIL(&jobs->snap_jobs, s, tailq);
		mutex_unlock(&jobs->dlock);
		wait_event(s->cond_queue, s->avail);
	} else {
		TAILQ_INSERT_TAIL(&jobs->snap_jobs, s, tailq);
		mutex_unlock(&jobs->dlock);
		ret = wait_event_timeout(s->cond_queue, s->avail, msecs_to_jiffies(timeout_ms));
		// ret < 0, error
		// ret = 0, timeout
		// ret = 1, condition true
	}

	if (s->avail)
		ret = 0;
	else
		ret = -1;

	if (!ret) {
		*blk = s->blk;
	} else {
		mutex_lock(&jobs->dlock);
		if (s->blk != VB_INVALID_HANDLE)
			vb_release_block(s->blk);
		TAILQ_REMOVE(&jobs->snap_jobs, s, tailq);
		mutex_unlock(&jobs->dlock);
		pr_err("Mod(%s) Grp(%d) Chn(%d), jobs wait(%d) work(%d) done(%d)\n"
			, sys_get_modname(chn.enModId), chn.s32DevId, chn.s32ChnId
			, FIFO_SIZE(&jobs->waitq), FIFO_SIZE(&jobs->workq), FIFO_SIZE(&jobs->doneq));
	}

	kfree(s);
	return ret;
}
EXPORT_SYMBOL_GPL(base_get_chn_buffer);

/* base_mod_jobs_init: initialize the jobs.
 *
 * @param jobs: vb jobs.
 * @param waitq_depth: the depth for waitq.
 * @param workq_depth: the depth for workq.
 * @param doneq_depth: the depth for doneq.
 */
void base_mod_jobs_init(struct vb_jobs_t *jobs, uint8_t waitq_depth, uint8_t workq_depth, uint8_t doneq_depth)
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
	FIFO_INIT(&jobs->waitq, waitq_depth);
	FIFO_INIT(&jobs->workq, workq_depth);
	FIFO_INIT(&jobs->doneq, doneq_depth);
	TAILQ_INIT(&jobs->snap_jobs);
	jobs->inited = true;
}
EXPORT_SYMBOL_GPL(base_mod_jobs_init);

/* mod_jobs_exit: end the jobs and release all resources.
 *
 * @param jobs: vb jobs.
 */
void base_mod_jobs_exit(struct vb_jobs_t *jobs)
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
	FIFO_EXIT(&jobs->waitq);
	while (!FIFO_EMPTY(&jobs->workq)) {
		FIFO_POP(&jobs->workq, &vb);
		vb_release_block((VB_BLK)vb);
	}
	FIFO_EXIT(&jobs->workq);
	mutex_unlock(&jobs->lock);
	mutex_destroy(&jobs->lock);

	mutex_lock(&jobs->dlock);
	while (!FIFO_EMPTY(&jobs->doneq)) {
		FIFO_POP(&jobs->doneq, &vb);
		vb_release_block((VB_BLK)vb);
	}
	FIFO_EXIT(&jobs->doneq);

	TAILQ_FOREACH_SAFE(s, &jobs->snap_jobs, tailq, s_tmp)
	TAILQ_REMOVE(&jobs->snap_jobs, s, tailq);
	mutex_unlock(&jobs->dlock);
	mutex_destroy(&jobs->dlock);
	jobs->inited = false;
}
EXPORT_SYMBOL_GPL(base_mod_jobs_exit);

/* mod_jobs_enque_work: Put job into work.
 *     Move vb from waitq into workq and put into driver.
 *
 * @param jobs: vb jobs.
 * @return: CVI_SUCCESS if OK.
 */
struct cvi_buffer *base_mod_jobs_enque_work(struct vb_jobs_t *jobs)
{
	struct vb_s *vb;
	int32_t ret = 0;

	if (jobs == NULL) {
		pr_err("[%p] job is NULL.\n", __builtin_return_address(0));
		return NULL;
	}

	mutex_lock(&jobs->lock);
	if (FIFO_EMPTY(&jobs->waitq)) {
		mutex_unlock(&jobs->lock);
		pr_err("waitq is empty.\n");
		return NULL;
	}
	if (FIFO_FULL(&jobs->workq)) {
		mutex_unlock(&jobs->lock);
		pr_err("workq is full.\n");
		return NULL;
	}

	FIFO_POP(&jobs->waitq, &vb);
	FIFO_PUSH(&jobs->workq, vb);
	mutex_unlock(&jobs->lock);

	pr_debug("phy-addr(%llx).\n", vb->phy_addr);

	if (ret != 0) {
		pr_err("qbuf error\n");
		return NULL;
	}
	return &vb->buf;
}
EXPORT_SYMBOL_GPL(base_mod_jobs_enque_work);

/* mod_jobs_waitq_empty: if waitq is empty
 *
 * @param jobs: vb jobs.
 * @return: TRUE if empty.
 */
bool base_mod_jobs_waitq_empty(struct vb_jobs_t *jobs)
{
	bool is_empty;

	if (jobs == NULL) {
		pr_err("[%p] job is NULL.\n", __builtin_return_address(0));
		return false;
	}

	mutex_lock(&jobs->lock);
	is_empty = FIFO_EMPTY(&jobs->waitq);
	mutex_unlock(&jobs->lock);

	return is_empty;
}
EXPORT_SYMBOL_GPL(base_mod_jobs_waitq_empty);

/* mod_jobs_workq_empty: if workq is empty
 *
 * @param jobs: vb jobs.
 * @return: TRUE if empty.
 */
bool base_mod_jobs_workq_empty(struct vb_jobs_t *jobs)
{
	bool is_empty;

	if (jobs == NULL) {
		pr_err("[%p] job is NULL.\n", __builtin_return_address(0));
		return false;
	}

	mutex_lock(&jobs->lock);
	is_empty = FIFO_EMPTY(&jobs->workq);
	mutex_unlock(&jobs->lock);

	return is_empty;
}
EXPORT_SYMBOL_GPL(base_mod_jobs_workq_empty);

/* mod_jobs_waitq_pop: pop out from waitq.
 *
 * @param jobs: vb jobs.
 * @return: VB_INVALID_HANDLE is not available; o/w, the VB_BLK.
 */
VB_BLK base_mod_jobs_waitq_pop(struct vb_jobs_t *jobs)
{
	struct vb_s *p;

	if (jobs == NULL) {
		pr_err("[%p] job is NULL.\n", __builtin_return_address(0));
		return VB_INVALID_HANDLE;
	}

	mutex_lock(&jobs->lock);
	if (FIFO_EMPTY(&jobs->waitq)) {
		mutex_unlock(&jobs->lock);
		pr_err("No more vb in waitq for dequeue.\n");
		return VB_INVALID_HANDLE;
	}
	FIFO_POP(&jobs->waitq, &p);
	mutex_unlock(&jobs->lock);
	return (VB_BLK)p;
}
EXPORT_SYMBOL_GPL(base_mod_jobs_waitq_pop);

/* mod_jobs_workq_pop: pop out from workq.
 *
 * @param jobs: vb jobs.
 * @return: VB_INVALID_HANDLE is not available; o/w, the VB_BLK.
 */
VB_BLK base_mod_jobs_workq_pop(struct vb_jobs_t *jobs)
{
	struct vb_s *p;

	if (jobs == NULL) {
		pr_err("Null parameter\n");
		return VB_INVALID_HANDLE;
	}

	mutex_lock(&jobs->lock);
	if (FIFO_EMPTY(&jobs->workq)) {
		mutex_unlock(&jobs->lock);
		pr_err("No more vb in workq for dequeue.\n");
		return VB_INVALID_HANDLE;
	}
	FIFO_POP(&jobs->workq, &p);
	mutex_unlock(&jobs->lock);
	return (VB_BLK)p;
}
EXPORT_SYMBOL_GPL(base_mod_jobs_workq_pop);

int32_t base_get_frame_info(PIXEL_FORMAT_E fmt, SIZE_S size, struct cvi_buffer *buf, u64 mem_base, u8 align)
{
	VB_CAL_CONFIG_S stVbCalConfig;
	u8 i = 0;

	COMMON_GetPicBufferConfig(size.u32Width, size.u32Height, fmt, DATA_BITWIDTH_8
		, COMPRESS_MODE_NONE, align, &stVbCalConfig);

	memset(buf, 0, sizeof(*buf));
	buf->size = size;
	buf->enPixelFormat = fmt;
	for (i = 0; i < stVbCalConfig.plane_num; ++i) {
		buf->phy_addr[i] = mem_base;
		buf->length[i] = ALIGN((i == 0) ? stVbCalConfig.u32MainYSize : stVbCalConfig.u32MainCSize,
					stVbCalConfig.u16AddrAlign);
		buf->stride[i] = (i == 0) ? stVbCalConfig.u32MainStride : stVbCalConfig.u32CStride;
		mem_base += buf->length[i];

		pr_debug("(%llx-%zu-%d)\n", buf->phy_addr[i], buf->length[i], buf->stride[i]);
	}

	return CVI_SUCCESS;
}
EXPORT_SYMBOL_GPL(base_get_frame_info);

/* _handle_snap: if there is get-frame request, hanlde it.
 *
 * @param chn: the channel where the blk is dequeued.
 * @param jobs: vb jobs.
 * @param blk: the VB_BLK to handle.
 */
static void _handle_snap(MMF_CHN_S chn, struct vb_jobs_t *jobs, VB_BLK blk)
{
	struct vb_s *p = (struct vb_s *)blk;
	struct vbq *doneq;
	struct snap_s *s, *s_tmp;

	if (jobs == NULL) {
		pr_err("handle snap fail, Null parameter\n");
		return;
	}

	if (!jobs->inited) {
		pr_err("handle snap fail, job not inited yet\n");
		return;
	}

	mutex_lock(&jobs->dlock);
	TAILQ_FOREACH_SAFE(s, &jobs->snap_jobs, tailq, s_tmp) {
		if (CHN_MATCH(&s->chn, &chn)) {
			TAILQ_REMOVE(&jobs->snap_jobs, s, tailq);
			s->blk = blk;
			atomic_fetch_add(1, &p->usr_cnt);
			atomic_long_fetch_or(BIT(CVI_ID_USER), &p->mod_ids);
			s->avail = CVI_TRUE;
			wake_up(&s->cond_queue);
			mutex_unlock(&jobs->dlock);
			return;
		}
	}

	doneq = &jobs->doneq;
	// check if there is a snap-queue
	if (FIFO_CAPACITY(doneq)) {
		if (FIFO_FULL(doneq)) {
			struct vb_s *vb = NULL;

			FIFO_POP(doneq, &vb);
			atomic_long_fetch_and(~BIT(chn.enModId), &vb->mod_ids);
			vb_release_block((VB_BLK)vb);
		}
		atomic_fetch_add(1, &p->usr_cnt);
		atomic_long_fetch_or(BIT(chn.enModId), &p->mod_ids);
		FIFO_PUSH(doneq, p);
	}
	mutex_unlock(&jobs->dlock);
}

/* vb_qbuf: queue vb into the specified channel.
 *     (src) Put into workq and driver.
 *     (dst) Put into waitq and sem_post
 *
 * @param chn: the channel to be queued.
 * @param chn_type: the chn is input(read) or output(write)
 * @param jobs: vb jobs.
 * @param blk: VB_BLK to be queued.
 */
int32_t vb_qbuf(MMF_CHN_S chn, enum CHN_TYPE_E chn_type, struct vb_jobs_t *jobs, VB_BLK blk)
{
	struct vb_s *vb = (struct vb_s *)blk;
	s32 ret = CVI_SUCCESS;

	pr_debug("%s dev(%d) chn(%d) chnType(%d): phy-addr(%lld) cnt(%d)\n",
		     sys_get_modname(chn.enModId), chn.s32DevId, chn.s32ChnId, chn_type,
		     vb->phy_addr, vb->usr_cnt.counter);

	if (!jobs) {
		pr_err("mod(%s), vb_qbuf fail, error, empty jobs\n", sys_get_modname(chn.enModId));
		return CVI_FAILURE;
	}
	if (!jobs->inited) {
		pr_err("mod(%s), vb_qbuf fail, jobs not initialized yet\n", sys_get_modname(chn.enModId));
		return CVI_FAILURE;
	}

	mutex_lock(&jobs->lock);
	if (chn_type == CHN_TYPE_OUT) {
		if (FIFO_FULL(&jobs->workq)) {
			mutex_unlock(&jobs->lock);
			pr_err("%s workq is full. drop new one.\n"
				     , sys_get_modname(chn.enModId));
			return -ENOBUFS;
		}
		vb->buf.dev_num = chn.s32ChnId;
		FIFO_PUSH(&jobs->workq, vb);
	} else {
		if (FIFO_FULL(&jobs->waitq)) {
			mutex_unlock(&jobs->lock);
			pr_err("%s waitq is full. drop new one.\n"
				     , sys_get_modname(chn.enModId));
			return -ENOBUFS;
		}
		FIFO_PUSH(&jobs->waitq, vb);
		up(&jobs->sem);
	}
	mutex_unlock(&jobs->lock);

	atomic_fetch_add(1, &vb->usr_cnt);
	atomic_long_fetch_or(BIT(chn.enModId), &vb->mod_ids);
	return ret;
}
EXPORT_SYMBOL_GPL(vb_qbuf);

/* vb_dqbuf: dequeue vb from the specified channel(driver).
 *
 * @param chn: the channel to be dequeued.
 * @param jobs: vb jobs.
 * @param blk: the VB_BLK dequeued.
 * @return: status of operation. CVI_SUCCESS if OK.
 */
int32_t vb_dqbuf(MMF_CHN_S chn, struct vb_jobs_t *jobs, VB_BLK *blk)
{
	struct vb_s *p;

	if (blk == NULL) {
		pr_err("[%p] blk is NULL.\n", __builtin_return_address(0));
		return CVI_FAILURE;
	}
	*blk = VB_INVALID_HANDLE;
	if (jobs == NULL) {
		pr_err("[%p] job is NULL.\n", __builtin_return_address(0));
		return CVI_FAILURE;
	}
	if (!jobs->inited) {
		pr_err("mod(%s), vb_qbuf fail, jobs not initialized yet\n", sys_get_modname(chn.enModId));
		return CVI_FAILURE;
	}

	mutex_lock(&jobs->lock);
	// get vb from workq which is done.
	if (FIFO_EMPTY(&jobs->workq)) {
		mutex_unlock(&jobs->lock);
		pr_err("%s ChnId(%d) No more vb for dequeue.\n",
			     sys_get_modname(chn.enModId), chn.s32ChnId);
		return CVI_FAILURE;
	}
	FIFO_POP(&jobs->workq, &p);
	mutex_unlock(&jobs->lock);
	*blk = (VB_BLK)p;
	atomic_long_fetch_and(~BIT(chn.enModId), &p->mod_ids);

	return CVI_SUCCESS;
}
EXPORT_SYMBOL_GPL(vb_dqbuf);

/* vb_done_handler: called when vb on specified chn is ready for delivery.
 *    Get vb from chn and deliver to its binding dsts if available;
 *    O/W, release back to vb_pool.
 *
 * @param chn: the chn which has vb to be released
 * @param chn_type: for modules which has both in/out.
 *                True: module generates(output) vb.
 *                False: module take(input) vb.
 * @param jobs: vb jobs.
 * @param blk: VB_BLK.
 * @return: status of operation. CVI_SUCCESS if OK.
 */
int32_t vb_done_handler(MMF_CHN_S chn, enum CHN_TYPE_E chn_type, struct vb_jobs_t *jobs, VB_BLK blk)
{
	MMF_BIND_DEST_S stBindDest;
	s32 ret;
	u8 i;
	MOD_ID_E id;

	if (chn_type == CHN_TYPE_OUT) {
		_handle_snap(chn, jobs, blk);
		if (bind_get_dst(&chn, &stBindDest) == CVI_SUCCESS) {
			for (i = 0; i < stBindDest.u32Num; ++i) {
				id = stBindDest.astMmfChn[i].enModId;
				if (base_recv_cb[id]) {
					ret = (*base_recv_cb[id])(stBindDest.astMmfChn[i], blk);
					if (ret != CVI_SUCCESS) {
						pr_info("%s base_recv_cb fail.",
							sys_get_modname(stBindDest.astMmfChn[i].enModId));
						if (chn.enModId == CVI_ID_VDEC)
							return ret;
					}
				}
				//vb_qbuf(stBindDest.astMmfChn[i], CHN_TYPE_IN, jobs, blk);
				pr_debug(" Mod(%s) chn(%d) dev(%d) -> Mod(%s) chn(%d) dev(%d)\n"
					     , sys_get_modname(chn.enModId), chn.s32ChnId, chn.s32DevId
					     , sys_get_modname(stBindDest.astMmfChn[i].enModId)
					     , stBindDest.astMmfChn[i].s32ChnId
					     , stBindDest.astMmfChn[i].s32DevId);
			}
		} else {
			// release if not found
			pr_debug("Mod(%s) chn(%d) dev(%d) src no dst release\n"
				     , sys_get_modname(chn.enModId), chn.s32ChnId, chn.s32DevId);
		}
	} else {
		pr_debug("Mod(%s) chn(%d) dev(%d) dst out release\n"
			     , sys_get_modname(chn.enModId), chn.s32ChnId, chn.s32DevId);
	}
	ret = vb_release_block(blk);

	return ret;
}
EXPORT_SYMBOL_GPL(vb_done_handler);

