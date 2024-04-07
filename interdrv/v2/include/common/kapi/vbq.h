#ifndef __VBQ_H__
#define __VBQ_H__

#include "vb.h"

struct snap_s {
	TAILQ_ENTRY(snap_s) tailq;

	//pthread_cond_t cond;
	wait_queue_head_t cond_queue;
	MMF_CHN_S chn;
	VB_BLK blk;
	CVI_BOOL avail;
};

/*
 * lock: lock for waitq/workq.
 * waitq: the queue of VB_BLK waiting to be done. For TDM modules, such as VPSS.
 * workq: the queue of VB_BLK module is working on.
 * dlock: lock for doneq.
 * doneq: the queue of VB_BLK to be taken. Size decided by u32Depth.
 * sem: sem to notify waitq is updated.
 * snap_jobs: the req to get frame for this chn.
 */
struct vb_jobs_t {
	struct mutex lock;
	struct vbq waitq;
	struct vbq workq;
	struct mutex dlock;
	struct vbq doneq;
	struct semaphore sem;
	TAILQ_HEAD(snap_q, snap_s) snap_jobs;
	uint8_t inited;
};

typedef int32_t (*VBQ_RECV_CB)(MMF_CHN_S chn, VB_BLK blk);

CVI_VOID base_register_recv_cb(MOD_ID_E enModId, VBQ_RECV_CB cb);
CVI_VOID base_unregister_recv_cb(MOD_ID_E enModId);

CVI_S32 base_fill_videoframe2buffer(MMF_CHN_S chn, const VIDEO_FRAME_INFO_S *pstVideoFrame,
	struct cvi_buffer *buf);
CVI_S32 base_get_chn_buffer(MMF_CHN_S chn, struct vb_jobs_t *jobs, VB_BLK *blk, CVI_S32 timeout_ms);
void base_mod_jobs_init(struct vb_jobs_t *jobs, uint8_t waitq_depth, uint8_t workq_depth, uint8_t doneq_depth);
void base_mod_jobs_exit(struct vb_jobs_t *jobs);
struct cvi_buffer *base_mod_jobs_enque_work(struct vb_jobs_t *jobs);
bool base_mod_jobs_waitq_empty(struct vb_jobs_t *jobs);
bool base_mod_jobs_workq_empty(struct vb_jobs_t *jobs);
VB_BLK base_mod_jobs_waitq_pop(struct vb_jobs_t *jobs);
VB_BLK base_mod_jobs_workq_pop(struct vb_jobs_t *jobs);
int32_t base_get_frame_info(PIXEL_FORMAT_E fmt, SIZE_S size, struct cvi_buffer *buf, u64 mem_base, u8 align);
int32_t vb_qbuf(MMF_CHN_S chn, enum CHN_TYPE_E chn_type, struct vb_jobs_t *jobs, VB_BLK blk);
int32_t vb_dqbuf(MMF_CHN_S chn, struct vb_jobs_t *jobs, VB_BLK *blk);
int32_t vb_done_handler(MMF_CHN_S chn, enum CHN_TYPE_E chn_type, struct vb_jobs_t *jobs, VB_BLK blk);


#endif  /* __CVI_BASE_CTX_H__ */
