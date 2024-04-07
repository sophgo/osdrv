#ifndef __VDEC_H__
#define __VDEC_H__

#include <linux/cvi_type.h>
#include <linux/cvi_defines.h>

#define DISPLAY_QUEUE_SIZE 32
#define MAX_VDEC_FRM_NUM 32

#define MAX_DEC_PIC_WIDTH               4096
#define MAX_DEC_PIC_HEIGHT              2304


typedef struct _vdec_dbg_ {
    CVI_S32 startFn;
    CVI_S32 endFn;
    CVI_S32 dbgMask;
    CVI_S32 currMask;
    CVI_CHAR dbgDir[CVI_VDEC_STR_LEN];
} vdec_dbg;

extern uint32_t VDEC_LOG_LV;

#define CVI_VDEC_FUNC_COND(FLAG, FUNC)			\
    do {                                    \
        if (VDEC_LOG_LV & (FLAG)) {    \
            FUNC;                            \
        }                                    \
    } while (0)


#define CVI_VDEC_PRNT(msg, ...)	\
            pr_info(msg, ##__VA_ARGS__)

#define CVI_VDEC_ERR(msg, ...)	\
    do {    \
        if (VDEC_LOG_LV & CVI_VDEC_MASK_ERR) \
        pr_err("[ERR] %s = %d, "msg, __func__, __LINE__, ## __VA_ARGS__); \
    } while (0)
#define CVI_VDEC_WARN(msg, ...)	\
    do {    \
        if (VDEC_LOG_LV & CVI_VDEC_MASK_WARN) \
        pr_warn("[WARN] %s = %d, "msg, __func__, __LINE__, ## __VA_ARGS__); \
    } while (0)
#define CVI_VDEC_DISP(msg, ...)	\
    do {    \
        if (VDEC_LOG_LV & CVI_VDEC_MASK_DISP) \
        pr_notice("[DISP] %s = %d, "msg, __func__, __LINE__, ## __VA_ARGS__); \
    } while (0)
#define CVI_VDEC_INFO(msg, ...)	\
    do {    \
        if (VDEC_LOG_LV & CVI_VDEC_MASK_INFO) \
        pr_info("[INFO] %s = %d, "msg, __func__, __LINE__, ## __VA_ARGS__); \
    } while (0)
#define CVI_VDEC_MEM(msg, ...)	\
    do {    \
        if (VDEC_LOG_LV & CVI_VDEC_MASK_MEM) \
        pr_info("[MEM] %s = %d, "msg, __func__, __LINE__, ## __VA_ARGS__); \
    } while (0)
#define CVI_VDEC_API(msg, ...)	\
    do {    \
        if (VDEC_LOG_LV & CVI_VDEC_MASK_API) \
        pr_info("[API] %s = %d, "msg, __func__, __LINE__, ## __VA_ARGS__); \
    } while (0)
#define CVI_VDEC_TRACE(msg, ...)	\
    do {    \
        if (VDEC_LOG_LV & CVI_VDEC_MASK_TRACE) \
        pr_debug("[TRACE] %s = %d, "msg, __func__, __LINE__, ## __VA_ARGS__); \
    } while (0)
#define CVI_VDEC_PERF(msg, ...)		\
    do { \
        if (VDEC_LOG_LV & CVI_VDEC_MASK_PERF) \
        pr_notice("[PERF] %s = %d, "msg, __func__, __LINE__, ## __VA_ARGS__); \
    } while (0)

#define CVI_TRACE_VDEC(level, fmt, ...)                                           \
    CVI_TRACE(level, CVI_ID_VDEC, "%s:%d:%s(): " fmt, __FILENAME__, __LINE__, __func__, ##__VA_ARGS__)


typedef struct _vdec_chn_context {
    VDEC_CHN VdChn;
    CVI_U64 totalDecTime;
    VDEC_CHN_ATTR_S ChnAttr;
    VDEC_CHN_PARAM_S ChnParam;
    VDEC_CHN_STATUS_S stStatus;
    VIDEO_FRAME_INFO_S *VideoFrameArray;
    CVI_U32 VideoFrameArrayNum;
    CVI_S8 display_queue[DISPLAY_QUEUE_SIZE];
    CVI_U32 w_idx;
    CVI_U32 r_idx;
    CVI_U32 seqNum;
    struct mutex display_queue_lock;
    struct mutex status_lock;
    struct mutex chnShmMutex;
    CVI_VOID *pHandle;
    CVI_BOOL bHasVbPool;
    VDEC_CHN_POOL_S vbPool;
    VB_BLK vbBLK[MAX_VDEC_FRM_NUM];
    cviBufInfo FrmArray[MAX_VDEC_FRM_NUM];
    CVI_U32 FrmNum;
    CVI_BOOL bStartRecv;
    struct cvi_vdec_vb_ctx *pVbCtx;

    CVI_U64 u64LastSendStreamTimeStamp;
    CVI_U64 u64LastGetFrameTimeStamp;
    CVI_U32 u32SendStreamCnt;
    CVI_U32 u32GetFrameCnt;
    VIDEO_FRAME_INFO_S stVideoFrameInfo;
    VCODEC_PERF_FPS_S stFPS;
    struct mutex jpdLock;
} vdec_chn_context;

typedef struct _vdec_context {
    vdec_chn_context *chn_handle[VDEC_MAX_CHN_NUM];
    VDEC_MOD_PARAM_S g_stModParam;
} vdec_context;

#define CVI_VDEC_NO_INPUT -10
#define CVI_VDEC_INPUT_ERR -11

#define VDEC_TIME_BLOCK_MODE (-1)
#define VDEC_RET_TIMEOUT (-2)
#define VDEC_TIME_TRY_MODE (0)
#define VDEC_DEFAULT_MUTEX_MODE VDEC_TIME_BLOCK_MODE

// below should align to cv183x_vcodec.h
#define VPU_MISCDEV_NAME "/dev/cvi-vcodec"
#define VCODEC_VENC_SHARE_MEM_SIZE (0x30000) // 192k
#define VCODEC_VDEC_SHARE_MEM_SIZE (0x8000) // 32k
#define VCODEC_SHARE_MEM_SIZE                                                  \
    (VCODEC_VENC_SHARE_MEM_SIZE + VCODEC_VDEC_SHARE_MEM_SIZE)
#ifndef SEC_TO_MS
#define SEC_TO_MS (1000)
#endif
#ifndef ALIGN
#define ALIGN(x, a) (((x) + ((a)-1)) & ~((a)-1))
#endif
#define UNUSED(x) ((void)(x))

#define IF_WANNA_DISABLE_BIND_MODE()                                           \
    ((pVbCtx->currBindMode == CVI_TRUE) &&                                 \
     (pVbCtx->enable_bind_mode == CVI_FALSE))
#define IF_WANNA_ENABLE_BIND_MODE()                                            \
    ((pVbCtx->currBindMode == CVI_FALSE) &&                                \
     (pVbCtx->enable_bind_mode == CVI_TRUE))


#endif
