#ifndef __U_CVI_VPSS_CTX_H__
#define __U_CVI_VPSS_CTX_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/cvi_defines.h>
#include <linux/cvi_comm_vpss.h>
#include <linux/cvi_comm_gdc.h>
#include <linux/cvi_comm_region.h>
#include <linux/cvi_base_ctx.h>
#include "base_ctx.h"
#include "vpss_hal.h"

#define VPSS_ONLINE_JOB_NUM 2

enum handler_state {
	HANDLER_STATE_STOP = 0,
	HANDLER_STATE_RUN,
	HANDLER_STATE_SUSPEND,
	HANDLER_STATE_RESUME,
	HANDLER_STATE_MAX,
};

struct VPSS_GRP_WORK_STATUS_S {
	u32 u32RecvCnt;
	u32 u32FRCRecvCnt;
	u32 u32LostCnt;
	u32 u32StartFailCnt; //start job fail cnt
	u32 u32CostTime; // current job cost time in us
	u32 u32MaxCostTime;
	u32 u32HwCostTime; // current job Hw cost time in us
	u32 u32HwMaxCostTime;
};

struct VPSS_CHN_WORK_STATUS_S {
	u32 u32SendOk; // send OK cnt after latest chn enable
	u64 u64PrevTime; // latest time (us)
	u32 u32FrameNum;  //The number of Frame in one second
	u32 u32RealFrameRate; // chn real time frame rate
};

struct VPSS_CHN_CFG {
	u8 isEnabled;
	u8 isMuted;
	u8 isDrop;
	VPSS_CHN_ATTR_S stChnAttr;
	VPSS_CROP_INFO_S stCropInfo;
	ROTATION_E enRotation;
	u32 blk_size;
	u32 align;
	RGN_HANDLE rgn_handle[RGN_MAX_LAYER_VPSS][RGN_MAX_NUM_VPSS]; //only overlay
	RGN_HANDLE coverEx_handle[RGN_COVEREX_MAX_NUM];
	RGN_HANDLE mosaic_handle[RGN_MOSAIC_MAX_NUM];
	struct cvi_rgn_cfg rgn_cfg[RGN_MAX_LAYER_VPSS];
	struct cvi_rgn_coverex_cfg rgn_coverex_cfg;
	struct cvi_rgn_mosaic_cfg rgn_mosaic_cfg;
	VPSS_SCALE_COEF_E enCoef;
	VPSS_DRAW_RECT_S stDrawRect;
	VPSS_CONVERT_S stConvert;
	u32 YRatio;
	VPSS_LDC_ATTR_S stLDCAttr;
	FISHEYE_ATTR_S stFishEyeAttr;
	u32 VbPool;
	VPSS_CHN_BUF_WRAP_S stBufWrap;
	u64 bufWrapPhyAddr;
	u32 u32BufWrapDepth;
	struct VPSS_CHN_WORK_STATUS_S stChnWorkStatus;

	// hw cfgs;
	u8 is_cfg_changed;
};

FIFO_HEAD(vpssjobq, cvi_vpss_job*);

struct cvi_vpss_ctx {
	VPSS_GRP VpssGrp;
	u8 isCreated;
	u8 isStarted;
	u8 online_from_isp;
	enum handler_state enHdlState;
	struct timespec64 time;
	struct mutex lock;
	VPSS_GRP_ATTR_S stGrpAttr;
	VPSS_CROP_INFO_S stGrpCropInfo;
	u8 chnNum;
	struct VPSS_CHN_CFG stChnCfgs[VPSS_MAX_CHN_NUM];
	struct crop_size frame_crop;
	s16 s16OffsetTop;
	s16 s16OffsetBottom;
	s16 s16OffsetLeft;
	s16 s16OffsetRight;
	struct VPSS_GRP_WORK_STATUS_S stGrpWorkStatus;
	void *pJobBuffer;
	struct vpssjobq jobq;
	struct cvi_vpss_hw_cfg hw_cfg;
	u8 is_cfg_changed;
	u8 is_copy_upsample;
};

#ifdef __cplusplus
}
#endif

#endif /* __U_CVI_VPSS_CTX_H__ */
