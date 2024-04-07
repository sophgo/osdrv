#ifndef __VENC_H__
#define __VENC_H__

#include <base_ctx.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/cvi_type.h>
#include <linux/cvi_defines.h>
#include <linux/cvi_comm_venc.h>
#include "vbq.h"
#include "enc_ctx.h"
#include "cvi_jpg_interface.h"
#include "cvi_float_point.h"

#define VENC_MEMSET memset

typedef enum _VENC_CHN_STATE_ {
    VENC_CHN_STATE_NONE = 0,
    VENC_CHN_STATE_INIT,
    VENC_CHN_STATE_START_ENC,
    VENC_CHN_STATE_STOP_ENC,
} VENC_CHN_STATE;

typedef struct _venc_frc {
    CVI_BOOL bFrcEnable;
    CVI_S32 srcFrameDur;
    CVI_S32 dstFrameDur;
    CVI_S32 srcTs;
    CVI_S32 dstTs;
} venc_frc;

typedef struct _venc_vfps {
    CVI_BOOL bVfpsEnable;
    CVI_S32 s32NumFrmsInOneSec;
    CVI_U64 u64prevSec;
    CVI_U64 u64StatTime;
} venc_vfps;

typedef struct _venc_vb_ctx {
	CVI_BOOL enable_bind_mode;
	CVI_BOOL currBindMode;
	struct task_struct *thread;
	struct vb_jobs_t vb_jobs;
	CVI_BOOL pause;
} venc_vb_ctx;

#define CVI_DEF_VFPFS_STAT_TIME 2
#define MAX_VENC_FRM_NUM 32

typedef struct _venc_chn_vars {
    CVI_U64 u64TimeOfSendFrame;
    CVI_U64 u64LastGetStreamTimeStamp;
    CVI_U64 u64LastSendFrameTimeStamp;
    CVI_U64 currPTS;
    CVI_U64 totalTime;
    CVI_S32 frameIdx;
    CVI_S32 s32RecvPicNum;
    CVI_S32 bind_event_fd;
    venc_frc frc;
    venc_vfps vfps;
    VENC_STREAM_S stStream;
    VENC_JPEG_PARAM_S stJpegParam;
    VENC_MJPEG_PARAM_S stMjpegParam;
    VENC_CHN_PARAM_S stChnParam;
    VENC_CHN_STATUS_S chnStatus;
    VENC_CU_PREDICTION_S cuPrediction;
    VENC_FRAME_PARAM_S frameParam;
    VENC_CHN_STATE chnState;
    USER_RC_INFO_S stUserRcInfo;
    VENC_SUPERFRAME_CFG_S stSuperFrmParam;
    struct semaphore sem_send;
    struct semaphore sem_release;
    CVI_BOOL bAttrChange;
    VENC_FRAMELOST_S frameLost;
    CVI_BOOL bHasVbPool;
    VENC_CHN_POOL_S vbpool;
    VB_BLK vbBLK[VB_COMM_POOL_MAX_CNT];
    cviBufInfo FrmArray[MAX_VENC_FRM_NUM];
    CVI_U32 FrmNum;
    CVI_U32 u32SendFrameCnt;
    CVI_U32 u32GetStreamCnt;
    CVI_S32 s32BindModeGetStreamRet;
    CVI_U32 u32FirstPixelFormat;
    CVI_BOOL bSendFirstFrm;
    CVI_U32 u32Stride[3];
    VIDEO_FRAME_INFO_S stFrameInfo;
    VENC_ROI_ATTR_S stRoiAttr[8];
    VCODEC_PERF_FPS_S stFPS;
} venc_chn_vars;

typedef struct _venc_chn_context {
    VENC_CHN VeChn;
    VENC_CHN_ATTR_S *pChnAttr;
    VENC_RC_PARAM_S rcParam;
    VENC_REF_PARAM_S refParam;
    VENC_FRAMELOST_S frameLost;
    VENC_H264_ENTROPY_S h264Entropy;
    VENC_H264_TRANS_S h264Trans;
    VENC_H265_TRANS_S h265Trans;
    VENC_H265_PU_S    h265PredUnit;
    VENC_SEARCH_WINDOW_S stSearchWindow;
    VENC_H264_SLICE_SPLIT_S h264Split;
    VENC_H265_SLICE_SPLIT_S h265Split;
    union {
        VENC_H264_VUI_S h264Vui;
        VENC_H265_VUI_S h265Vui;
    };
    VENC_H264_INTRA_PRED_S h264IntraPred;
    venc_enc_ctx encCtx;
    venc_chn_vars *pChnVars;
    venc_vb_ctx *pVbCtx;
    struct mutex chnMutex;
    struct mutex chnShmMutex;
    CVI_BOOL bSbSkipFrm;
    CVI_U32 jpgFrmSkipCnt;
    VIDEO_FRAME_INFO_S stVideoFrameInfo;
    CVI_BOOL bChnEnable;
    union {
        VENC_H264_DBLK_S h264Dblk;
        VENC_H265_DBLK_S h265Dblk;
    };
    VENC_CUSTOM_MAP_S customMap;
    VENC_H265_SAO_S h265Sao;
} venc_chn_context;

typedef struct _CVI_VENC_MODPARAM_S {
    VENC_MOD_VENC_S stVencModParam;
    VENC_MOD_H264E_S stH264eModParam;
    VENC_MOD_H265E_S stH265eModParam;
    VENC_MOD_JPEGE_S stJpegeModParam;
    VENC_MOD_RC_S stRcModParam;
} CVI_VENC_PARAM_MOD_S;

typedef struct _venc_context {
    venc_chn_context * chn_handle[VENC_MAX_CHN_NUM];
    CVI_U32 chn_status[VENC_MAX_CHN_NUM];
    CVI_VENC_PARAM_MOD_S ModParam;
} venc_context;


#define Q_TABLE_MAX 99
#define Q_TABLE_CUSTOM 50
#define Q_TABLE_MIN 0
#define Q_TABLE_DEFAULT 0 // 0 = backward compatible
#define SRC_FRAMERATE_DEF 30
#define SRC_FRAMERATE_MAX 240
#define SRC_FRAMERATE_MIN 1
#define DEST_FRAMERATE_DEF 30
#define DEST_FRAMERATE_MAX 60
#define DEST_FRAMERATE_MIN 1
#define CVI_VENC_NO_INPUT -10
#define CVI_VENC_INPUT_ERR -11
#define DUMP_YUV "dump_src.yuv"
#define DUMP_BS "dump_bs.bin"
#ifndef SEC_TO_MS
#define SEC_TO_MS 1000
#endif
#define USERDATA_MAX_DEFAULT 1024
#define USERDATA_MAX_LIMIT 65536
#define DEFAULT_NO_INPUTDATA_TIMEOUT_SEC (5)
#define BYPASS_SB_MODE (0)
#define CLK_ENABLE_REG_2_BASE (0x03002008)
#define VC_FAB_REG_9_BASE (0xb030024)

// below should align to cv183x_vcodec.h
#define VPU_MISCDEV_NAME "/dev/cvi-vcodec"
#define VCODEC_VENC_SHARE_MEM_SIZE (0x30000) // 192k
#define VCODEC_VDEC_SHARE_MEM_SIZE (0x8000) // 32k
#define VCODEC_SHARE_MEM_SIZE                                                  \
    (VCODEC_VENC_SHARE_MEM_SIZE + VCODEC_VDEC_SHARE_MEM_SIZE)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define FRC_TIME_SCALE 0xFFF0
#if SOFT_FLOAT
#define FLOAT_VAL_FRC_TIME_SCALE (0x477ff000)
#else
#define FLOAT_VAL_FRC_TIME_SCALE (FRC_TIME_SCALE)
#endif
#define FRC_TIME_OVERFLOW_OFFSET 0x40000000


#define SET_DEFAULT_RC_PARAM(RC)                                        \
    do {                                                                \
        (RC)->u32MaxIprop = CVI_H26X_MAX_I_PROP_DEFAULT;               \
        (RC)->u32MinIprop = CVI_H26X_MIN_I_PROP_DEFAULT;               \
        (RC)->u32MaxIQp = DEF_264_MAXIQP;                              \
        (RC)->u32MinIQp = DEF_264_MINIQP;                              \
        (RC)->u32MaxQp = DEF_264_MAXQP;                                \
        (RC)->u32MinQp = DEF_264_MINQP;                                \
    } while (0)

#define SET_COMMON_RC_PARAM(DEST, SRC)                                 \
    do {                                                               \
        (DEST)->u32MaxIprop = (SRC)->u32MaxIprop;                      \
        (DEST)->u32MinIprop = (SRC)->u32MinIprop;                      \
        (DEST)->u32MaxIQp = (SRC)->u32MaxIQp;                          \
        (DEST)->u32MinIQp = (SRC)->u32MinIQp;                          \
        (DEST)->u32MaxQp = (SRC)->u32MaxQp;                            \
        (DEST)->u32MinQp = (SRC)->u32MinQp;                            \
        (DEST)->s32MaxReEncodeTimes = (SRC)->s32MaxReEncodeTimes;      \
    } while (0)

#if 0
#define IF_WANNA_DISABLE_BIND_MODE()                                           \
    ((pVbCtx->currBindMode == CVI_TRUE) &&                                 \
     (pVbCtx->enable_bind_mode == CVI_FALSE))
#define IF_WANNA_ENABLE_BIND_MODE()                                            \
    ((pVbCtx->currBindMode == CVI_FALSE) &&                                \
     (pVbCtx->enable_bind_mode == CVI_TRUE))
#endif




#endif
