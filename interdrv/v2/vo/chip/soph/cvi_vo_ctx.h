#ifndef __U_CVI_VO_CTX_H__
#define __U_CVI_VO_CTX_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/cvi_vip.h>
#include <linux/cvi_defines.h>
#include <linux/cvi_comm_vo.h>
#include <linux/cvi_comm_region.h>
#include "vbq.h"

#define VO_SHARE_MEM_SIZE           (0x2000)

struct cvi_vo_wbc_ctx {
	u8 is_wbc_enable;
	u8 is_wbc_src_cfg;
	u8 is_wbc_attr_cfg;
	u8 is_odma_enable;
	VO_WBC_SRC_S stWbcSrc;
	VO_WBC_ATTR_S stWbcAttr;
	VO_WBC_MODE_E enWbcMode;
	u32 u32Depth;

	struct vb_jobs_t wbc_jobs;
	struct mutex wbc_lock;
	struct task_struct *thread;
	wait_queue_head_t wq;
	unsigned int event;

	struct list_head qbuf_list;
	struct list_head dqbuf_list;
	spinlock_t	qbuf_lock;
	spinlock_t	dqbuf_lock;
	u8 qbuf_num;

	u32 u32DoneCnt;
	u32 u32FrameNum;
	u32 u32WbcFrameRate;
	u8 bIsDrop;
	u32 u32OdmaFifoFull;
};

struct cvi_vo_chn_ctx {
	u8 is_chn_enable;
	u8 bHide;
	u8 bPause;
	u8 bRefresh;
	u8 bStep;
	u8 bStepTrigger;
	VO_CHN_ATTR_S stChnAttr;
	VO_CHN_ZOOM_ATTR_S stChnZoomAttr;
	VO_CHN_BORDER_ATTR_S stChnBorder;
	VO_CHN_PARAM_S stChnParam;
	VO_CHN_MIRROR_TYPE enChnMirror;
	u32 u32SrcWidth;
	u32 u32SrcHeight;
	ROTATION_E enRotation;
	u8 bIsDrop;

	struct vb_jobs_t chn_jobs;
	struct mutex gdc_lock;

	u64 u64DisplayPts;
	u64 u64PreDonePts;
	u32 u32FrameNum;
	u32 u32SrcFrameNum;
	u32 u32ChnFrameRate;
	u32 u32ChnSrcFrameRate;
	u32 u32FrameRateUserSet;
	u32 u32FrameIndex;
	u32 u32Threshold;

	struct {
		u64 paddr;
		void *vaddr;
	} mesh;
};

struct cvi_vo_layer_ctx {
	VO_LAYER VoLayer;
	u8 is_layer_enable;
	u8 bLayerUpdate;
	u8 bIsDrop;
	VO_VIDEO_LAYER_ATTR_S stLayerAttr;
	u32 u32DisBufLen;
	s32 proc_amp[PROC_AMP_MAX];
	s32 s32BindDevId;
	struct cvi_vo_chn_ctx astChnCtx[VO_MAX_CHN_NUM];

	RGN_HANDLE rgn_handle[RGN_MAX_NUM_VO];
	RGN_HANDLE rgn_coverEx_handle[RGN_COVEREX_MAX_NUM];
	struct cvi_rgn_cfg rgn_cfg;
	struct cvi_rgn_coverex_cfg rgn_coverex_cfg;

	struct mutex layer_lock;
	struct task_struct *thread;
	wait_queue_head_t	wq;
	unsigned int		event;
	spinlock_t			list_lock;
	struct list_head	list_wait;
	struct list_head	list_work;
	struct list_head	list_done;
	struct vb_jobs_t 	layer_jobs;

	u32 u32FrameNum;
	u32 u32SrcFrameNum;
	u32 u32LayerFrameRate;
	u32 u32LayerSrcFrameRate;
	u32 u32FrameIndex;
	u32 u32DoneCnt;
	u32 u32Toleration;
	u64 u64DisplayPts;
	u64 u64PreDonePts;
	u32 u32BwFail;
	u32 u32OsdBwFail;


	struct{
		u32 left;
		u32 top;
		u32 width;
		u32 height;
	} rect_crop;

};

struct cvi_vo_dev_ctx {
	u8 is_dev_enable;
	VO_PUB_ATTR_S stPubAttr;
	s32 s32BindLayerId;
};

struct cvi_vo_ctx {
	u8 bSuspend;
	struct cvi_vo_dev_ctx astDevCtx[VO_MAX_DEV_NUM];
	struct cvi_vo_layer_ctx astLayerCtx[VO_MAX_LAYER_NUM];
	struct cvi_vo_wbc_ctx astWbcCtx[VO_MAX_WBC_NUM];
};

#ifdef __cplusplus
}
#endif

#endif /* __U_CVI_VO_CTX_H__ */
