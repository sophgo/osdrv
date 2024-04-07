#ifdef ENABLE_DEC
#include "cvi_vdec.h"
#include <base_ctx.h>
#include <linux/cvi_buffer.h>
#include <linux/cvi_defines.h>
#include "module_common.h"
#include "cvi_vc_drv_proc.h"
#include "cvi_jpg_interface.h"
#include "cvi_h265_interface.h"
#include <linux/cvi_comm_vdec.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <uapi/linux/sched/types.h>
#include "cvi_vc_drv.h"

#include "vdec.h"
#include "drv_file.h"


vdec_dbg vdecDbg;
vdec_context *vdec_handle;
#ifdef USE_VB_POOL
extern int32_t vb_create_pool(struct cvi_vb_pool_cfg *config);
extern int32_t vb_get_config(struct cvi_vb_cfg *pstVbConfig);
extern VB_BLK vb_get_block_with_id(VB_POOL poolId, uint32_t u32BlkSize,
                   MOD_ID_E modId);
extern uint64_t vb_handle2phys_addr(VB_BLK blk);
extern VB_BLK vb_phys_addr2handle(uint64_t u64PhyAddr);
extern VB_POOL vb_handle2pool_id(VB_BLK blk);
extern int32_t vb_release_block(VB_BLK blk);
extern int32_t vb_destroy_pool(VB_POOL poolId);
// TODO: need to refine:
struct cvi_vdec_vb_ctx vdec_vb_ctx[VENC_MAX_CHN_NUM];
#endif


uint32_t VDEC_LOG_LV = CVI_VDEC_MASK_ERR;
module_param(VDEC_LOG_LV, int, 0644);
extern wait_queue_head_t tVdecWaitQueue[];


static DEFINE_MUTEX(g_vdec_handle_mutex);
static DEFINE_MUTEX(jpdLock);

static CVI_U64 get_current_time(CVI_VOID)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    struct timespec64 ts;
#else
    struct timespec ts;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
    ktime_get_ts64(&ts);
#else
    ktime_get_ts(&ts);
#endif
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000; // in ms
}

static CVI_VOID cviChangeVdecMask(CVI_S32 frameIdx)
{
    vdec_dbg *pDbg = &vdecDbg;

    pDbg->currMask = pDbg->dbgMask;
    if (pDbg->startFn >= 0) {
        if (frameIdx >= pDbg->startFn && frameIdx <= pDbg->endFn)
            pDbg->currMask = pDbg->dbgMask;
        else
            pDbg->currMask = CVI_VDEC_MASK_ERR;
    }

    CVI_VDEC_TRACE("currMask = 0x%X\n", pDbg->currMask);
}

static CVI_S32 check_vdec_chn_handle(VDEC_CHN VdChn)
{
    if (vdec_handle == NULL) {
        CVI_VDEC_ERR("Call VDEC Destroy before Create chn:%d, failed\n", VdChn);
        return CVI_ERR_VDEC_UNEXIST;
    }

    if (vdec_handle->chn_handle[VdChn] == NULL) {
        CVI_VDEC_ERR("VDEC Chn #%d haven't created !\n", VdChn);
        return CVI_ERR_VDEC_INVALID_CHNID;
    }

    return CVI_SUCCESS;
}


static void cvi_h26x_set_output_format(cviDecOnePicCfg *pdopc,
                       PIXEL_FORMAT_E enPixelFormat)
{
    if (pdopc == NULL)
        return;

    switch (enPixelFormat) {
    case PIXEL_FORMAT_NV12:
        pdopc->cbcrInterleave = 1;
        pdopc->nv21 = 0;
        break;
    case PIXEL_FORMAT_NV21:
        pdopc->cbcrInterleave = 1;
        pdopc->nv21 = 1;
        break;
    default:
        pdopc->cbcrInterleave = 0;
        pdopc->nv21 = 0;
        break;
    }
}

static CVI_S32 cvi_h264_decode(void *pHandle, PIXEL_FORMAT_E enPixelFormat,
                   const VDEC_STREAM_S *pstStream,
                   CVI_S32 s32MilliSec)
{
    cviDecOnePicCfg dopc, *pdopc = &dopc;
    int decStatus = 0;

    pdopc->bsAddr = pstStream->pu8Addr;
    pdopc->bsLen = pstStream->u32Len;
    pdopc->bEndOfStream = pstStream->bEndOfStream;
    pdopc->pts = pstStream->u64PTS;
    pdopc->dts = pstStream->u64DTS;
    cvi_h26x_set_output_format(pdopc, enPixelFormat);

    decStatus = cviVDecDecOnePic(pHandle, pdopc, s32MilliSec);

    return decStatus;
}

static CVI_S32 cvi_h265_decode(void *pHandle, PIXEL_FORMAT_E enPixelFormat,
                   const VDEC_STREAM_S *pstStream,
                   CVI_S32 s32MilliSec)
{
    cviDecOnePicCfg dopc, *pdopc = &dopc;
    int decStatus = 0;

    pdopc->bsAddr = pstStream->pu8Addr;
    pdopc->bsLen = pstStream->u32Len;
    pdopc->bEndOfStream = pstStream->bEndOfStream;
    pdopc->pts = pstStream->u64PTS;
    pdopc->dts = pstStream->u64DTS;
    cvi_h26x_set_output_format(pdopc, enPixelFormat);

    decStatus = cviVDecDecOnePic(pHandle, pdopc, s32MilliSec);

    return decStatus;
}


static void cviSetVideoFrameInfo(VIDEO_FRAME_INFO_S *pstVideoFrame,
                 cviDispFrameCfg *pdfc)
{
    VIDEO_FRAME_S *pstVFrame = &pstVideoFrame->stVFrame;

    pstVFrame->enPixelFormat =
        (pdfc->cbcrInterleave) ?
                  ((pdfc->nv21) ? PIXEL_FORMAT_NV21 : PIXEL_FORMAT_NV12) :
                  PIXEL_FORMAT_YUV_PLANAR_420;

    pstVFrame->pu8VirAddr[0] = pdfc->addr[0];
    pstVFrame->pu8VirAddr[1] = pdfc->addr[1];
    pstVFrame->pu8VirAddr[2] = pdfc->addr[2];

    pstVFrame->u64PhyAddr[0] = pdfc->phyAddr[0];
    pstVFrame->u64PhyAddr[1] = pdfc->phyAddr[1];
    pstVFrame->u64PhyAddr[2] = pdfc->phyAddr[2];

    pstVFrame->u32Width = pdfc->width;
    pstVFrame->u32Height = pdfc->height;

    pstVFrame->u32Stride[0] = pdfc->strideY;
    pstVFrame->u32Stride[1] = pdfc->strideC;
    pstVFrame->u32Stride[2] = pdfc->cbcrInterleave ? 0 : pdfc->strideC;
    pstVFrame->u32Length[0] = pdfc->length[0];
    pstVFrame->u32Length[1] = pdfc->length[1];
    pstVFrame->u32Length[2] = pdfc->length[2];

    if (pdfc->bCompressFrame) {
        pstVFrame->enCompressMode = COMPRESS_MODE_FRAME;
        pstVFrame->pu8ExtVirtAddr = pdfc->addr[3];
        pstVFrame->u64ExtPhyAddr = pdfc->phyAddr[3];
        pstVFrame->u32ExtLength = pdfc->length[3];
    } else {
        pstVFrame->enCompressMode = COMPRESS_MODE_NONE;
        pstVFrame->pu8ExtVirtAddr = 0;
        pstVFrame->u64ExtPhyAddr = 0;
        pstVFrame->u32ExtLength = 0;
    }

    pstVFrame->s16OffsetTop = 0;
    pstVFrame->s16OffsetBottom = 0;
    pstVFrame->s16OffsetLeft = 0;
    pstVFrame->s16OffsetRight = 0;
    pstVFrame->u64PTS = pdfc->pts;
    pstVFrame->u64DTS = pdfc->dts;
    pstVFrame->pPrivateData = (void *)(uintptr_t)pdfc->indexFrameDisplay;
    pstVFrame->u8Endian = pdfc->endian;
    pstVFrame->u8InterlacedFrame = pdfc->interlacedFrame;
    pstVFrame->u8PicType = pdfc->picType;
    pstVFrame->u32SeqenceNo = pdfc->seqenceNo;
}

static CVI_S32 cviSetVideoChnAttrToProc(VDEC_CHN VdChn,
                    VIDEO_FRAME_INFO_S *psFrameInfo,
                    CVI_U64 u64DecHwTime)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    vdec_chn_context *pChnHandle = NULL;

    CVI_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (psFrameInfo == NULL) {
        CVI_VDEC_ERR("psFrameInfo is NULL\n");
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    memcpy(&pChnHandle->stVideoFrameInfo, psFrameInfo,
           sizeof(VIDEO_FRAME_INFO_S));
    pChnHandle->stFPS.u64HwTime = u64DecHwTime;

    return s32Ret;
}

static CVI_S32 cviSetVdecFpsToProc(VDEC_CHN VdChn, CVI_BOOL bSendStream)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    CVI_U64 u64CurTime = get_current_time();
    vdec_chn_context *pChnHandle = NULL;

    CVI_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    if (bSendStream) {
        if ((u64CurTime - pChnHandle->u64LastSendStreamTimeStamp) >
            SEC_TO_MS) {
            pChnHandle->stFPS.u32InFPS =
                (CVI_U32)((pChnHandle->u32SendStreamCnt *
                       SEC_TO_MS) /
                      ((CVI_U32)(u64CurTime -
                             pChnHandle
                                 ->u64LastSendStreamTimeStamp)));
            pChnHandle->u64LastSendStreamTimeStamp = u64CurTime;
            pChnHandle->u32SendStreamCnt = 0;
        }
    } else {
        if ((u64CurTime - pChnHandle->u64LastGetFrameTimeStamp) >
            SEC_TO_MS) {
            pChnHandle->stFPS.u32OutFPS =
                (CVI_U32)((pChnHandle->u32GetFrameCnt *
                       SEC_TO_MS) /
                      ((CVI_U32)(u64CurTime -
                             pChnHandle
                                 ->u64LastGetFrameTimeStamp)));
            pChnHandle->u64LastGetFrameTimeStamp = u64CurTime;
            pChnHandle->u32GetFrameCnt = 0;
        }
    }

    return s32Ret;
}


static inline CVI_S32 checkTimeOutAndBusy(int s32Ret, int line)
{
    if (s32Ret == 0)
        return s32Ret;

    if ((s32Ret == ETIMEDOUT) || (s32Ret == EBUSY)) {
        CVI_VDEC_TRACE("mutex timeout and retry\n");
        return  CVI_ERR_VDEC_BUSY;
    }

    CVI_VDEC_ERR("vdec mutex error[%d], line = %d\n", s32Ret, line);
    return CVI_ERR_VDEC_ERR_VDEC_MUTEX;
}


static CVI_S32 cviVdec_Mutex_Unlock(struct mutex *__mutex)
{
    mutex_unlock(__mutex);
    return 0;
}

static CVI_S32 cviVdec_Mutex_Lock(struct mutex *__restrict __mutex,
                  CVI_S32 s32MilliSec, CVI_S32 *s32CostTime)
{
    CVI_S32 s32RetCostTime;
    CVI_S32 s32RetCheck;
    CVI_U64 u64StartTime, u64EndTime;

    u64StartTime = get_current_time();
    if (s32MilliSec < 0) {
        //block mode
        s32RetCheck = mutex_lock_interruptible(__mutex);
        u64EndTime = get_current_time();
        s32RetCostTime = u64EndTime - u64StartTime;
    } else if (s32MilliSec == 0) {
        //trylock
        if (mutex_trylock(__mutex)) {
            s32RetCheck = 0;
        } else {
            s32RetCheck = EBUSY;
        }
        s32RetCostTime = 0;
    } else {
        //timelock
        int wait_cnt_ms = 1;
        while (mutex_is_locked(__mutex)) {
            set_current_state(TASK_INTERRUPTIBLE);
            schedule_timeout(usecs_to_jiffies(1000));
            if (wait_cnt_ms >= s32MilliSec) {
                break;
            }
            wait_cnt_ms++;
        }
        if (mutex_trylock(__mutex)) {
            s32RetCheck = 0;
        } else {
            s32RetCheck = EBUSY;
        }
        u64EndTime = get_current_time();
        s32RetCostTime = u64EndTime - u64StartTime;
    }
    //calculate cost lock time

    //check the mutex validity
    if (s32RetCheck != 0) {
        CVI_VDEC_ERR("mutex lock error[%d]\n", s32RetCheck);
    }

    if (s32CostTime != NULL)
        *s32CostTime = s32RetCostTime;
    return s32RetCheck;
}


#if 0
static CVI_U32 vdec_dump_queue_status(VDEC_CHN VdChn)
{

    vdec_chn_context *pChnHandle = vdec_handle->chn_handle[VdChn];

    if (vdec_handle == NULL) {
        CVI_VDEC_ERR("Call VENC Destroy before Create, failed\n");
        return CVI_ERR_VDEC_UNEXIST;
    }

    if (vdec_handle->chn_handle[VdChn] == NULL) {
        CVI_VDEC_ERR("VDEC Chn #%d haven't created !\n", VdChn);
        return CVI_ERR_VDEC_INVALID_CHNID;
    }

    for (int i = 0; i < pChnHandle->ChnAttr.u32FrameBufCnt; i++) {
        CVI_VDEC_INFO("FbArray %d, flag %d\n", i, pChnHandle->VideoFrameArray[i].stVFrame.u32FrameFlag);
    }

    for (int i = 0; i < DISPLAY_QUEUE_SIZE; i++) {
        CVI_VDEC_INFO("DisQueue %d, idx %d\n", i, pChnHandle->display_queue[i]);
    }

    return CVI_SUCCESS;

}
#endif

static CVI_S32 get_avail_fb(VDEC_CHN VdChn, VIDEO_FRAME_INFO_S *VideoFrameArray)
{
    vdec_chn_context *pChnHandle = NULL;
    CVI_S32 s32Ret = check_vdec_chn_handle(VdChn);
    CVI_S32 avail_fb_idx = -1;
    CVI_U32 i = 0;

    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return avail_fb_idx;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    for (i = 0; i < pChnHandle->VideoFrameArrayNum; i++) {
        if (VideoFrameArray[i].stVFrame.u32FrameFlag == 0) {
            avail_fb_idx = i;
            break;
        }
    }

    return avail_fb_idx;
}

static CVI_S32 insert_display_queue(CVI_U32 w_idx, CVI_S32 fb_idx,
                    CVI_S8 *display_queue)
{
    if (display_queue[w_idx] != -1)
        return CVI_ERR_VDEC_ILLEGAL_PARAM;
    CVI_VDEC_INFO("insert_display_queue, w_idx %d, fb_idx %d\n", w_idx,
              fb_idx);
    display_queue[w_idx] = fb_idx;

    return CVI_SUCCESS;
}

static CVI_S32 allocate_vdec_frame(vdec_chn_context *pChnHandle, CVI_U32 idx)
{
    CVI_S32 s32Ret = CVI_SUCCESS;

    VDEC_CHN_ATTR_S *pstAttr = &pChnHandle->ChnAttr;
    VDEC_CHN_PARAM_S *pstParam = &pChnHandle->ChnParam;

    VIDEO_FRAME_S *pstVFrame = &(pChnHandle->VideoFrameArray[idx].stVFrame);
    VB_CAL_CONFIG_S stVbCfg;
    PIXEL_FORMAT_E enPixelFormat =
        (pstAttr->enType == PT_JPEG || pstAttr->enType == PT_MJPEG) ?
                  pstParam->enPixelFormat :
                  PIXEL_FORMAT_YUV_PLANAR_420;

    CVI_VDEC_API("\n");

    memset(&stVbCfg, 0, sizeof(stVbCfg));
    VDEC_GetPicBufferConfig(pstAttr->enType, pstAttr->u32PicWidth,
                pstAttr->u32PicHeight, enPixelFormat,
                DATA_BITWIDTH_8, COMPRESS_MODE_NONE, &stVbCfg);

    pstVFrame->enCompressMode = COMPRESS_MODE_NONE;
    pstVFrame->enPixelFormat = enPixelFormat;
    pstVFrame->enVideoFormat = VIDEO_FORMAT_LINEAR;
    pstVFrame->enColorGamut = COLOR_GAMUT_BT709;
    pstVFrame->u32Width = pstAttr->u32PicWidth;
    pstVFrame->u32Height = pstAttr->u32PicHeight;
    pstVFrame->u32Stride[0] = stVbCfg.u32MainStride;
    pstVFrame->u32Stride[1] =
        (stVbCfg.plane_num > 1) ? stVbCfg.u32CStride : 0;
    pstVFrame->u32Stride[2] =
        (stVbCfg.plane_num > 2) ? stVbCfg.u32CStride : 0;
    pstVFrame->u32TimeRef = 0;
    pstVFrame->u64PTS = 0;
    pstVFrame->enDynamicRange = DYNAMIC_RANGE_SDR8;
    pstVFrame->u32FrameFlag = 0;

    CVI_VDEC_DISP(
        "u32Stride[0] = %d, u32Stride[1] = %d, u32Stride[2] = %d\n",
        pstVFrame->u32Stride[0], pstVFrame->u32Stride[1],
        pstVFrame->u32Stride[2]);

    if (pstVFrame->u32Width % DEFAULT_ALIGN) {
        CVI_VDEC_INFO("u32Width is not algined to %d\n", DEFAULT_ALIGN);
    }

    if (pstAttr->enType == PT_JPEG || pstAttr->enType == PT_MJPEG) {
        VB_POOL hPicVbPool = VB_INVALID_POOLID;
        VB_BLK blk;

        if (vdec_handle->g_stModParam.enVdecVBSource != VB_SOURCE_USER) {
        #if 0
            blk = vb_get_block_with_id(VB_INVALID_POOLID, stVbCfg.u32VBSize, CVI_ID_VDEC);
        #else
            return CVI_SUCCESS;
        #endif
        } else {
            hPicVbPool = pChnHandle->vbPool.hPicVbPool;
            blk = vb_get_block_with_id(
                hPicVbPool, stVbCfg.u32VBSize, CVI_ID_VDEC);
        }

        if (blk == VB_INVALID_HANDLE) {
            CVI_VDEC_ERR("Can't acquire vb block\n");
            return CVI_ERR_VDEC_NOMEM;
        }

        pChnHandle->vbBLK[idx] = blk;

        CVI_VDEC_INFO("alloc size %d, %d x %d\n", stVbCfg.u32VBSize,
                  pstAttr->u32PicWidth, pstAttr->u32PicHeight);

        pChnHandle->VideoFrameArray[idx].u32PoolId =
            vb_handle2pool_id(blk);

        pstVFrame->u32Length[0] = stVbCfg.u32MainYSize;
        pstVFrame->u64PhyAddr[0] = vb_handle2phys_addr(blk);

        if (stVbCfg.plane_num > 1) {
            pstVFrame->u32Length[1] = stVbCfg.u32MainCSize;
            pstVFrame->u64PhyAddr[1] = pstVFrame->u64PhyAddr[0] +
                           ALIGN(stVbCfg.u32MainYSize,
                             stVbCfg.u16AddrAlign);
        } else {
            pstVFrame->u32Length[1] = 0;
            pstVFrame->u64PhyAddr[1] = 0;
            pstVFrame->pu8VirAddr[1] = NULL;
        }

        if (stVbCfg.plane_num > 2) {
            pstVFrame->u32Length[2] = stVbCfg.u32MainCSize;
            pstVFrame->u64PhyAddr[2] = pstVFrame->u64PhyAddr[1] +
                           ALIGN(stVbCfg.u32MainCSize,
                             stVbCfg.u16AddrAlign);
        } else {
            pstVFrame->u32Length[2] = 0;
            pstVFrame->u64PhyAddr[2] = 0;
            pstVFrame->pu8VirAddr[2] = NULL;
        }

        CVI_VDEC_DISP("phy addr(%llx, %llx, %llx\n",
                  pstVFrame->u64PhyAddr[0],
                  pstVFrame->u64PhyAddr[1],
                  pstVFrame->u64PhyAddr[2]);

        CVI_VDEC_DISP("vir addr(%p, %p, %p\n", pstVFrame->pu8VirAddr[0],
                  pstVFrame->pu8VirAddr[1],
                  pstVFrame->pu8VirAddr[2]);

        pChnHandle->FrmArray[idx].phyAddr = pstVFrame->u64PhyAddr[0];
        pChnHandle->FrmArray[idx].size = stVbCfg.u32VBSize;
        pChnHandle->FrmArray[idx].virtAddr = pstVFrame->pu8VirAddr[0];
        pChnHandle->FrmNum++;
    }

    return s32Ret;
}

CVI_S32 _CVI_VDEC_InitHandle(void)
{
    CVI_S32 s32Ret = CVI_SUCCESS;

    if (MUTEX_LOCK(&g_vdec_handle_mutex) != 0) {
        CVI_VDEC_ERR("can not lock g_vdec_handle_mutex\n");
        return CVI_FAILURE;
    }

    if (vdec_handle == NULL) {
        vdec_handle = MEM_CALLOC(1, sizeof(vdec_context));

        if (vdec_handle == NULL) {
            MUTEX_UNLOCK(&g_vdec_handle_mutex);
            return CVI_ERR_VDEC_NOMEM;
        }

        memset(vdec_handle, 0x00, sizeof(vdec_context));
        vdec_handle->g_stModParam.enVdecVBSource =
            VB_SOURCE_COMMON; // Default is VB_SOURCE_COMMON
        vdec_handle->g_stModParam.u32MiniBufMode = 0;
        vdec_handle->g_stModParam.u32ParallelMode = 0;
        vdec_handle->g_stModParam.stVideoModParam.u32MaxPicHeight =
            2160;
        vdec_handle->g_stModParam.stVideoModParam.u32MaxPicWidth = 4096;
        vdec_handle->g_stModParam.stVideoModParam.u32MaxSliceNum = 200;
        vdec_handle->g_stModParam.stPictureModParam.u32MaxPicHeight =
            8192;
        vdec_handle->g_stModParam.stPictureModParam.u32MaxPicWidth =
            8192;
    }

    MUTEX_UNLOCK(&g_vdec_handle_mutex);

    return s32Ret;
}


int cviVdecDrvInit(void)
{
    CVI_S32 s32Ret = CVI_SUCCESS;

    s32Ret = _CVI_VDEC_InitHandle();
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("venc_context\n");
        s32Ret = CVI_ERR_VDEC_NOMEM;
    }
    return s32Ret;
}

void cviVdecDrvDeinit(void)
{
    if(vdec_handle)
        MEM_FREE(vdec_handle);
    vdec_handle = NULL;

    CVIJpgUninit();

    return;
}


static CVI_S32 _cvi_vdec_AllocVbBuf(vdec_chn_context *pChnHandle, void *arg)
{
    stCviCb_HostAllocFB *pHostAllocFB = (stCviCb_HostAllocFB *)arg;

    if (vdec_handle->g_stModParam.enVdecVBSource == VB_SOURCE_PRIVATE) {
        if (pChnHandle->bHasVbPool == false) {

            struct cvi_vb_pool_cfg stVbPoolCfg;

            stVbPoolCfg.blk_size = pHostAllocFB->iFrmBufSize;
            stVbPoolCfg.blk_cnt = pHostAllocFB->iFrmBufNum;
            stVbPoolCfg.remap_mode = VB_REMAP_MODE_NONE;

            #ifdef USE_VB_POOL
            pChnHandle->vbPool.hPicVbPool =
                vb_create_pool(&stVbPoolCfg);

            if (pChnHandle->vbPool.hPicVbPool ==
                VB_INVALID_POOLID) {
                CVI_VDEC_ERR("CVI_VB_CreatePool failed !\n");
                return 0;
            }

            pChnHandle->bHasVbPool = true;
                #endif

            CVI_VDEC_TRACE(
                "Create Private Pool: %d, u32BlkSize=0x%x,  u32BlkCnt=%d\n",
                pChnHandle->vbPool.hPicVbPool,
                stVbPoolCfg.blk_size, stVbPoolCfg.blk_cnt);
        }
    }

    if (pChnHandle->bHasVbPool == false) {
        CVI_VDEC_ERR("pChnHandle->bHasVbPool == false\n");
        return 0;
    }

    if (pHostAllocFB->iFrmBufNum > MAX_VDEC_FRM_NUM) {
        CVI_VDEC_ERR("iFrmBufNum > %d\n", MAX_VDEC_FRM_NUM);
        return 0;
    }

    #ifdef USE_VB_POOL
    for (j = 0; j < pHostAllocFB->iFrmBufNum; j++) {
        CVI_U64 u64PhyAddr;

        pChnHandle->vbBLK[j] =
            vb_get_block_with_id(pChnHandle->vbPool.hPicVbPool,
                         pHostAllocFB->iFrmBufSize,
                         CVI_ID_VDEC);
        if (pChnHandle->vbBLK[j] == VB_INVALID_HANDLE) {
            CVI_VDEC_ERR("CVI_VB_GetBlockwithID failed !\n");
            CVI_VDEC_ERR(
                "Frame isn't enough. Need %d frames, size 0x%x. Now only %d frames\n",
                pHostAllocFB->iFrmBufNum,
                pHostAllocFB->iFrmBufSize, j);
            return 0;
        }

        u64PhyAddr = vb_handle2phys_addr(pChnHandle->vbBLK[j]);

        pChnHandle->FrmArray[j].phyAddr = u64PhyAddr;
        pChnHandle->FrmArray[j].size = pHostAllocFB->iFrmBufSize;

        CVI_VDEC_TRACE(
            "CVI_VB_GetBlockwithID, VbBlk = %d, u64PhyAddr = 0x%llx, virtAddr = %p\n",
            (CVI_S32)pChnHandle->vbBLK[j], (long long)u64PhyAddr,
            pChnHandle->FrmArray[j].virtAddr);
    }
    #endif

    pChnHandle->FrmNum = pHostAllocFB->iFrmBufNum;
#if 0
    cviVDecAttachFrmBuf((void *)pChnHandle->pHandle,
                (void *)pChnHandle->FrmArray,
                pHostAllocFB->iFrmBufNum);
#endif
    return 1;
}

static CVI_S32 _cvi_vdec_FreeVbBuf(vdec_chn_context *pChnHandle)
{
    if (vdec_handle->g_stModParam.enVdecVBSource == VB_SOURCE_PRIVATE) {
        CVI_VDEC_ERR("not support VB_SOURCE_PRIVATE yet\n");
        return 0;
    }

    CVI_VDEC_TRACE("\n");
    #ifdef USE_VB_POOL
    if (pChnHandle->ChnAttr.enType == PT_H264 ||
        pChnHandle->ChnAttr.enType == PT_H265) {
        if (pChnHandle->bHasVbPool == true) {
            CVI_U32 i = 0;
            VB_BLK blk;

            for (i = 0; i < pChnHandle->FrmNum; i++) {
                blk = vb_phys_addr2handle(
                    pChnHandle->FrmArray[i].phyAddr);

                if (blk != VB_INVALID_HANDLE) {
                    vb_release_block(blk);
                }
            }
        } else {
            CVI_VDEC_ERR("bHasVbPool = false\n");
            return 0;
        }
    }
    #endif

    return 1;
}

static CVI_S32 _cvi_vdec_GetDispQ_Count(vdec_chn_context *pChnHandle)
{
    CVI_S32 s32Count = 0;

    CVI_U32 i = 0;

    for (i = 0; i < pChnHandle->VideoFrameArrayNum; i++) {
        VIDEO_FRAME_S *pstCurFrame;

        pstCurFrame = &pChnHandle->VideoFrameArray[i].stVFrame;

        if (pstCurFrame->u32FrameFlag == 1) {
            s32Count++;
        }
    }

    CVI_VDEC_TRACE("s32Count=%d\n", s32Count);

    return s32Count;
}

static CVI_S32 _cvi_vdec_CallbackFunc(unsigned int VdChn, unsigned int CbType,
                      void *arg)
{
    CVI_S32 s32Ret = 0, s32Check = CVI_SUCCESS;
    vdec_chn_context *pChnHandle = NULL;

    s32Check = check_vdec_chn_handle(VdChn);
    if (s32Check != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    CVI_VDEC_TRACE("VdChn = %d, CbType= %d\n", VdChn, CbType);

    pChnHandle = vdec_handle->chn_handle[VdChn];

    switch (CbType) {
    case CVI_H26X_DEC_CB_AllocFB:
        if (arg == NULL) {
            CVI_VDEC_ERR("arg == NULL\n");
            return s32Ret;
        }

        if (_cvi_vdec_AllocVbBuf(pChnHandle, arg) != 1) {
            return s32Ret;
        }
        s32Ret = 1;
        break;
    case CVI_H26X_DEC_CB_FreeFB:
        if (_cvi_vdec_FreeVbBuf(pChnHandle) != 1) {
            return s32Ret;
        }
        s32Ret = 1;
        break;
    case CVI_H26X_DEC_CB_GET_DISPQ_COUNT:
        s32Ret = _cvi_vdec_GetDispQ_Count(pChnHandle);
        break;
    default:
        CVI_VDEC_ERR("Unsupported cbType: %d\n", CbType);
        break;
    }

    return s32Ret;
}

#define VDEC_NO_FRAME_IDX 0xFFFFFFFF
CVI_BOOL _cvi_vdec_FindBlkInfo(vdec_chn_context *pChnHandle, CVI_U64 u64PhyAddr,
                   VB_BLK *pVbBLK, CVI_U32 *pFrmIdx)
{
    CVI_U32 i = 0;

    if ((pChnHandle == NULL) || (pVbBLK == NULL) || (pFrmIdx == NULL))
        return false;

    *pVbBLK = VB_INVALID_HANDLE;
    *pFrmIdx = VDEC_NO_FRAME_IDX;

    for (i = 0; i < pChnHandle->FrmNum; i++) {
        if ((pChnHandle->FrmArray[i].phyAddr <= u64PhyAddr) &&
            (pChnHandle->FrmArray[i].phyAddr +
             pChnHandle->FrmArray[i].size) > u64PhyAddr) {
            *pVbBLK = pChnHandle->vbBLK[i];
            *pFrmIdx = i;
            break;
        }
    }

    if (i == pChnHandle->FrmNum) {
        CVI_VDEC_ERR("Can't find BLK !\n");
        return false;
    }

    return true;
}

CVI_S32 _cvi_vdec_FindFrameIdx(vdec_chn_context *pChnHandle,
                   const VIDEO_FRAME_INFO_S *pstFrameInfo)
{
    CVI_U32 i = 0;

    if ((pChnHandle == NULL) || (pstFrameInfo == NULL))
        return -1;

    for (i = 0; i < pChnHandle->VideoFrameArrayNum; i++) {
        VIDEO_FRAME_S *pstCurFrame;

        pstCurFrame = &pChnHandle->VideoFrameArray[i].stVFrame;

        if ((pstCurFrame->u32FrameFlag == 1) &&
            (pstCurFrame->u64PhyAddr[0] ==
             pstFrameInfo->stVFrame.u64PhyAddr[0]) &&
            (pstCurFrame->u64PhyAddr[1] ==
             pstFrameInfo->stVFrame.u64PhyAddr[1]) &&
            (pstCurFrame->u64PhyAddr[2] ==
             pstFrameInfo->stVFrame.u64PhyAddr[2])) {
            return i;
        }
    }

    return -1;
}

static CVI_VOID cviGetDebugConfigFromDecProc(void)
{
    extern proc_debug_config_t tVdecDebugConfig;
    proc_debug_config_t *ptDecProcDebugLevel = &tVdecDebugConfig;
    vdec_dbg *pDbg = &vdecDbg;

    CVI_VDEC_TRACE("\n");

    if (ptDecProcDebugLevel == NULL) {
        CVI_VDEC_ERR("ptDecProcDebugLevel is NULL\n");
        return;
    }

    memset(pDbg, 0, sizeof(vdec_dbg));

    pDbg->dbgMask = ptDecProcDebugLevel->u32DbgMask;
    if (pDbg->dbgMask == CVI_VDEC_NO_INPUT ||
        pDbg->dbgMask == CVI_VDEC_INPUT_ERR)
        pDbg->dbgMask = CVI_VDEC_MASK_ERR;
    else
        pDbg->dbgMask |= CVI_VDEC_MASK_ERR;

    pDbg->currMask = pDbg->dbgMask;
    pDbg->startFn = ptDecProcDebugLevel->u32StartFrmIdx;
    pDbg->endFn = ptDecProcDebugLevel->u32EndFrmIdx;
    strcpy(pDbg->dbgDir, ptDecProcDebugLevel->cDumpPath);

    cviChangeVdecMask(0);
}


static CVI_S32 free_vdec_frame(VDEC_CHN_ATTR_S *pstAttr,
                   VIDEO_FRAME_INFO_S *pstVideoFrame)
{
    VIDEO_FRAME_S *pstVFrame = &pstVideoFrame->stVFrame;

    VB_BLK blk;

    if (pstAttr->enType == PT_JPEG || pstAttr->enType == PT_MJPEG) {
        blk = vb_phys_addr2handle(pstVFrame->u64PhyAddr[0]);
        if (blk != VB_INVALID_HANDLE) {
            vb_release_block(blk);
        }
    }

    return CVI_SUCCESS;
}

static void cvi_jpeg_set_frame_info(VIDEO_FRAME_S *pVideoFrame,
                    CVIFRAMEBUF *pCVIFrameBuf)
{
    int c_h_shift = 0; // chroma height shift

    pVideoFrame->u32Width = pCVIFrameBuf->width;
    pVideoFrame->u32Height = pCVIFrameBuf->height;
    pVideoFrame->u32Stride[0] = pCVIFrameBuf->strideY;
    pVideoFrame->u32Stride[1] = pCVIFrameBuf->strideC;
    pVideoFrame->u32Stride[2] = pCVIFrameBuf->strideC;
    pVideoFrame->s16OffsetTop = 0;
    pVideoFrame->s16OffsetBottom = 0;
    pVideoFrame->s16OffsetLeft = 0;
    pVideoFrame->s16OffsetRight = 0;

    switch (pCVIFrameBuf->format) {
    case CVI_FORMAT_400:
        pVideoFrame->enPixelFormat = PIXEL_FORMAT_YUV_400;
        pVideoFrame->u32Stride[2] = pCVIFrameBuf->strideC;
        break;
    case CVI_FORMAT_422:
        if(pCVIFrameBuf->packedFormat == CVI_PACKED_FORMAT_422_YUYV) {
            pVideoFrame->enPixelFormat = PIXEL_FORMAT_YUYV;
        } else if(pCVIFrameBuf->packedFormat == CVI_PACKED_FORMAT_422_UYVY) {
            pVideoFrame->enPixelFormat = PIXEL_FORMAT_UYVY;
        } else if(pCVIFrameBuf->packedFormat == CVI_PACKED_FORMAT_422_YVYU) {
            pVideoFrame->enPixelFormat = PIXEL_FORMAT_YVYU;
        } else if(pCVIFrameBuf->packedFormat == CVI_PACKED_FORMAT_422_VYUY) {
            pVideoFrame->enPixelFormat = PIXEL_FORMAT_VYUY;
        } else if (pCVIFrameBuf->packedFormat == CVI_PACKED_FORMAT_NONE){
            if(pCVIFrameBuf->chromaInterleave == CVI_CBCR_INTERLEAVE) {
                pVideoFrame->enPixelFormat = PIXEL_FORMAT_NV16;
            } else if(pCVIFrameBuf->chromaInterleave == CVI_CRCB_INTERLEAVE) {
                pVideoFrame->enPixelFormat = PIXEL_FORMAT_NV61;
            } else
                pVideoFrame->enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_422;
        }
        pVideoFrame->u32Stride[2] = pCVIFrameBuf->strideC;
        break;
    case CVI_FORMAT_444:
        pVideoFrame->enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_444;
        pVideoFrame->u32Stride[2] = pCVIFrameBuf->strideC;
        break;
    case CVI_FORMAT_420:
    default:
        c_h_shift = 1;
        if (pCVIFrameBuf->chromaInterleave == CVI_CBCR_INTERLEAVE) {
            pVideoFrame->enPixelFormat = PIXEL_FORMAT_NV12;
            pVideoFrame->u32Stride[2] = 0;
        } else if (pCVIFrameBuf->chromaInterleave ==
               CVI_CRCB_INTERLEAVE) {
            pVideoFrame->enPixelFormat = PIXEL_FORMAT_NV21;
            pVideoFrame->u32Stride[2] = 0;
        } else { // CVI_CBCR_SEPARATED
            pVideoFrame->enPixelFormat =
                PIXEL_FORMAT_YUV_PLANAR_420;
            pVideoFrame->u32Stride[2] = pCVIFrameBuf->strideC;
        }
        break;
    }

    pVideoFrame->u32Length[0] = pCVIFrameBuf->vbY.size;
    pVideoFrame->u32Length[1] = pCVIFrameBuf->vbCb.size;
    pVideoFrame->u32Length[2] = pCVIFrameBuf->vbCr.size;
    pVideoFrame->u64PhyAddr[0] = pCVIFrameBuf->vbY.phys_addr;
    pVideoFrame->pu8VirAddr[0] = pCVIFrameBuf->vbY.virt_addr;
    pVideoFrame->u64PhyAddr[1] = pCVIFrameBuf->vbCb.phys_addr;
    pVideoFrame->pu8VirAddr[1] = pCVIFrameBuf->vbCb.virt_addr;
    pVideoFrame->u64PhyAddr[2] = pCVIFrameBuf->vbCr.phys_addr;
    pVideoFrame->pu8VirAddr[2] = pCVIFrameBuf->vbCr.virt_addr;

    CVI_VDEC_INFO(
        "jpeg dec fmt:%d %dx%d, strideY %d, strideC %d, sizeY %d, sizeC %d\n",
        pVideoFrame->enPixelFormat,
        pVideoFrame->u32Width, pVideoFrame->u32Height,
        pVideoFrame->u32Stride[0], pVideoFrame->u32Stride[1],
        pVideoFrame->u32Length[0], pVideoFrame->u32Length[1]);
#if 0

    {
        int i = 0;
        int writeLen = 0;
        int writeTotalLen = 0;
        drv_file_t *fpYuv = drv_fopen("/mnt/sd/drv_set_out.yuv", "wb");
        if (0 == fpYuv) {
            pr_err("Cann't create a file to write data\n");
            return 0;
        }
        pr_err("[drv x]u64PhyAddr %lu %lu %lu\n", pVideoFrame->u64PhyAddr[0],
         pVideoFrame->u64PhyAddr[1],pVideoFrame->u64PhyAddr[2]);

        int datLen = pVideoFrame->u32Width;
        unsigned char *addrY_v = (unsigned char *)(pVideoFrame->pu8VirAddr[0]);
        unsigned char *addrCb_v = (unsigned char *)(pVideoFrame->pu8VirAddr[1]);
        unsigned char *addrCr_v = (unsigned char *)(pVideoFrame->pu8VirAddr[2]);
        unsigned char *address = addrY_v;

        for (i = 0; i < pVideoFrame->u32Height; i++) {
            writeLen = drv_fwrite(address,
                      sizeof(unsigned char), datLen, fpYuv);
            writeTotalLen += writeLen;
            address = address + pVideoFrame->u32Stride[0];
        }

        int iChromaHeight = pVideoFrame->u32Height>>1;
        int iChromaWidth = pVideoFrame->u32Width>>1;
        pr_err("[drv]stride %lu %lu %lu\n", pVideoFrame->u32Stride[0],
         pVideoFrame->u32Stride[1],pVideoFrame->u32Stride[2]);

        pr_err("[drv]u32Length %lu %lu %lu\n", pVideoFrame->u32Length[0],
         pVideoFrame->u32Length[1],pVideoFrame->u32Length[2]);

        address = addrCb_v;

        writeLen = drv_fwrite((unsigned char *)address, sizeof(unsigned char),
                  iChromaHeight * pVideoFrame->u32Stride[1], fpYuv);

        address = addrCr_v;

        writeLen = drv_fwrite((unsigned char *)address, sizeof(unsigned char),
                  iChromaHeight * pVideoFrame->u32Stride[2], fpYuv);
        if (0 != fpYuv) {
            drv_fclose(fpYuv);
            fpYuv = NULL;
        }
    }
#endif

}

static CVI_S32 cvi_jpeg_decode(vdec_chn_context *pChnHandle, const VDEC_STREAM_S *pstStream,
                   VIDEO_FRAME_INFO_S *pstVideoFrame,
                   CVI_S32 s32MilliSec, CVI_U64 *pu64DecHwTime)
{
    int ret = CVI_SUCCESS;
    CVIPackedFormat enPackedFormat = CVI_PACKED_FORMAT_NONE;
    CVICbCrInterLeave enChromaInterLeave = CVI_CBCR_SEPARATED;
    CVIFrameFormat eOutFormat = CVI_FORMAT_BUTT;
    VDEC_CHN_PARAM_S *pstChnParam = &pChnHandle->ChnParam;

    CVIJpgHandle jpgHandle = { 0 };
    CVIDecConfigParam decConfig;
    CVIJpgConfig config;

    unsigned char *srcBuf;
    int readLen;
    CVIFRAMEBUF cviFrameBuf;

    memset(&decConfig, 0, sizeof(CVIDecConfigParam));
    memset(&config, 0, sizeof(CVIJpgConfig));

    /* read source file data */
    srcBuf = pstStream->pu8Addr;
    readLen = pstStream->u32Len;

    config.type = CVIJPGCOD_DEC;

    if (pstChnParam->enPixelFormat == PIXEL_FORMAT_NV12) {
        enChromaInterLeave = CVI_CBCR_INTERLEAVE;
        eOutFormat = CVI_FORMAT_420;
    }
    else if (pstChnParam->enPixelFormat == PIXEL_FORMAT_NV21) {
        enChromaInterLeave = CVI_CRCB_INTERLEAVE;
        eOutFormat = CVI_FORMAT_420;
    }
    else if (pstChnParam->enPixelFormat == PIXEL_FORMAT_NV16) {
        enChromaInterLeave = CVI_CBCR_INTERLEAVE;
        eOutFormat = CVI_FORMAT_422;
    }
    else if (pstChnParam->enPixelFormat == PIXEL_FORMAT_NV61) {
        enChromaInterLeave = CVI_CRCB_INTERLEAVE;
        eOutFormat = CVI_FORMAT_422;
    }
    else {
        enChromaInterLeave = CVI_CBCR_SEPARATED;
    }

    // 2'b00: Normal
    // 2'b10: CbCr interleave (e.g. NV12 in 4:2:0 or NV16 in 4:2:2)
    // 2'b11: CrCb interleave (e.g. NV21 in 4:2:0)
    memset(&(config.u.dec), 0, sizeof(CVIDecConfigParam));
    config.u.dec.dec_buf.format = eOutFormat;
    config.u.dec.dec_buf.packedFormat = enPackedFormat;
    config.u.dec.dec_buf.chromaInterleave = enChromaInterLeave;
    // ROI param
    config.u.dec.roiEnable = pstChnParam->stVdecPictureParam.s32ROIEnable;
    config.u.dec.roiWidth = pstChnParam->stVdecPictureParam.s32ROIWidth;
    config.u.dec.roiHeight = pstChnParam->stVdecPictureParam.s32ROIHeight;
    config.u.dec.roiOffsetX = pstChnParam->stVdecPictureParam.s32ROIOffsetX;
    config.u.dec.roiOffsetY = pstChnParam->stVdecPictureParam.s32ROIOffsetY;
    // Frame Partial Mode (DON'T SUPPORT)
    config.u.dec.usePartialMode = 0;
    // Rotation Angle (0, 90, 180, 270)
    config.u.dec.rotAngle = pstChnParam->stVdecPictureParam.s32RotAngle;
    // mirror direction (0-no mirror, 1-vertical, 2-horizontal,
    // 3-both)

    config.u.dec.mirDir = pstChnParam->stVdecPictureParam.s32MirDir;
    config.u.dec.iHorScaleMode = pstChnParam->stVdecPictureParam.u32HDownSampling;
    config.u.dec.iVerScaleMode = pstChnParam->stVdecPictureParam.u32VDownSampling;
    config.u.dec.iDataLen = readLen;
    //config.u.dec.dst_type = JPEG_MEM_MODULE;
    if(config.u.dec.dst_type == JPEG_MEM_EXTERNAL) {
        config.u.dec.dec_buf.vbY.phys_addr =
            pstVideoFrame->stVFrame.u64PhyAddr[0];
        config.u.dec.dec_buf.vbCb.phys_addr =
            pstVideoFrame->stVFrame.u64PhyAddr[1];
        config.u.dec.dec_buf.vbCr.phys_addr =
            pstVideoFrame->stVFrame.u64PhyAddr[2];
    }
    CVI_VDEC_INFO("iDataLen = %d\n", config.u.dec.iDataLen);

    config.s32ChnNum = pChnHandle->VdChn;
    /* Open JPU Devices */
    jpgHandle = CVIJpgOpen(config);
    if (jpgHandle == NULL) {
        CVI_VDEC_ERR("\nFailed to CVIJpgOpen\n");
        ret = CVI_ERR_VDEC_NULL_PTR;
        goto FIND_ERROR;
    }

    /* send jpeg data for decode or encode operator */
    ret = CVIJpgSendFrameData(jpgHandle, (void *)srcBuf, readLen,
                  s32MilliSec);

    if (ret != 0) {
        if ((ret == VDEC_RET_TIMEOUT) && (s32MilliSec >= 0)) {
            CVI_VDEC_TRACE("CVIJpgSendFrameData ret timeout\n");
            return CVI_ERR_VDEC_BUSY;

        } else {
            CVI_VDEC_ERR(
                "\nFailed to CVIJpgSendFrameData, ret = %d\n",
                ret);
            ret = CVI_ERR_VDEC_ERR_SEND_FAILED;
            goto FIND_ERROR;
        }
    }

    memset(&cviFrameBuf, 0, sizeof(CVIFRAMEBUF));

    ret = CVIJpgGetFrameData(jpgHandle, (unsigned char *)&cviFrameBuf,
                 sizeof(CVIFRAMEBUF),
                 (unsigned long int *)pu64DecHwTime);
    if (ret != 0) {
        CVI_VDEC_ERR("\nFailed to CVIJpgGetFrameData, ret = %d\n", ret);
        ret = CVI_ERR_VDEC_ERR_GET_FAILED;
        goto FIND_ERROR;
    }
    cvi_jpeg_set_frame_info(&pstVideoFrame->stVFrame, &cviFrameBuf);
    pChnHandle->pHandle = jpgHandle;
    return CVI_SUCCESS;

FIND_ERROR:
    if (jpgHandle != NULL) {
        CVIJpgReleaseFrameData(jpgHandle);
        CVIJpgClose(jpgHandle);
        jpgHandle = NULL;
    }

    return ret;
}


CVI_S32 cviVdecGetOutputFrameCount(VDEC_CHN VdChn)
{
    vdec_chn_context *pChnHandle = NULL;
    CVI_S32 s32Ret = CVI_SUCCESS;

    if (vdec_handle == NULL) {
        return -1;
    }

    if (vdec_handle->chn_handle[VdChn] == NULL) {
        return -2;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    if(pChnHandle->ChnAttr.enType == PT_JPEG
        || pChnHandle->ChnAttr.enType == PT_MJPEG)
        s32Ret = pChnHandle->stStatus.u32LeftPics;
    else
        s32Ret = cviVdecGetFrameDisplayCount(pChnHandle->pHandle);

    return s32Ret;
}
static CVI_S32 CVI_VDEC_Init(void)
{
    CVI_S32 ret = CVI_SUCCESS;

    CVI_VDEC_API("\n");
    /* initial JPU core */
    ret = CVIJpgInit();
    if (ret != 0) {
        CVI_VDEC_ERR("\nFailed to CVIJpgInit!!!\n");
        return CVI_ERR_VDEC_ERR_INIT;
    }

    cviGetDebugConfigFromDecProc();

    return ret;
}

#define MAX_VDEC_DISPLAYQ_NUM 32

CVI_S32 CVI_VDEC_CreateChn(VDEC_CHN VdChn, const VDEC_CHN_ATTR_S *pstAttr)
{
    vdec_chn_context *pChnHandle = NULL;
    CVI_S32 s32Ret = CVI_SUCCESS;

    CVI_VDEC_API("VdChn = %d\n", VdChn);

    CVI_VDEC_Init();

    vdec_handle->chn_handle[VdChn] =
        MEM_CALLOC(1, sizeof(vdec_chn_context));
    if (vdec_handle->chn_handle[VdChn] == NULL) {
        CVI_VDEC_ERR("Allocate chn_handle memory failed !\n");
        return CVI_ERR_VDEC_NOMEM;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    pChnHandle->VdChn = VdChn;
    #ifdef USE_VB_POOL
    pChnHandle->pVbCtx = &vdec_vb_ctx[VdChn];
    #endif

    memcpy(&pChnHandle->ChnAttr, pstAttr, sizeof(VDEC_CHN_ATTR_S));
    pChnHandle->VideoFrameArrayNum = MAX_VDEC_DISPLAYQ_NUM;


    pChnHandle->VideoFrameArray = MEM_CALLOC(pChnHandle->VideoFrameArrayNum,
                         sizeof(VIDEO_FRAME_INFO_S));

    if (pChnHandle->VideoFrameArray == NULL) {
        CVI_VDEC_ERR("Allocate VideoFrameArray memory failed !\n");
        return CVI_ERR_VDEC_NOMEM;
    }

    CVI_VDEC_TRACE("u32FrameBufCnt = %d\n",
               pChnHandle->ChnAttr.u32FrameBufCnt);
    CVI_VDEC_TRACE("VideoFrameArrayNum = %d\n",
               pChnHandle->VideoFrameArrayNum);

    memset(pChnHandle->display_queue, -1, DISPLAY_QUEUE_SIZE);

    MUTEX_INIT(&pChnHandle->display_queue_lock, 0);
    MUTEX_INIT(&pChnHandle->jpdLock, 0);
    MUTEX_INIT(&pChnHandle->status_lock, 0);
    MUTEX_INIT(&pChnHandle->chnShmMutex, &ma);

    if (pChnHandle->ChnAttr.enType == PT_JPEG ||
        pChnHandle->ChnAttr.enType == PT_MJPEG) {
    } else if (pChnHandle->ChnAttr.enType == PT_H264 ||
           pChnHandle->ChnAttr.enType == PT_H265) {
        cviInitDecConfig initDecCfg, *pInitDecCfg;

        pInitDecCfg = &initDecCfg;
        memset(pInitDecCfg, 0, sizeof(cviInitDecConfig));
        pInitDecCfg->codec = (pChnHandle->ChnAttr.enType == PT_H265) ?
                           CODEC_H265 :
                           CODEC_H264;
        pInitDecCfg->cviApiMode = API_MODE_SDK;

        pInitDecCfg->vbSrcMode =
            vdec_handle->g_stModParam.enVdecVBSource;
        pInitDecCfg->chnNum = VdChn;
        pInitDecCfg->bsBufferSize = pChnHandle->ChnAttr.u32StreamBufSize;
        pInitDecCfg->frameBufferCount = pChnHandle->ChnAttr.u32FrameBufCnt;

        if (pstAttr->enMode == VIDEO_MODE_STREAM)
            pInitDecCfg->BsMode = BS_MODE_INTERRUPT;
        else if (pstAttr->enMode == VIDEO_MODE_FRAME)
            pInitDecCfg->BsMode = BS_MODE_PIC_END;
        else
            return CVI_INVALID_VALUE;

        if (pstAttr->enCompressMode == COMPRESS_MODE_FRAME)
            pInitDecCfg->wtl_enable = 0;
        else
            pInitDecCfg->wtl_enable = 1;

        pInitDecCfg->cmdQueueDepth = pChnHandle->ChnAttr.u8CommandQueueDepth;
        s32Ret = cviVDecOpen(pInitDecCfg, &pChnHandle->pHandle);
        if (s32Ret != 0) {
            CVI_VDEC_ERR("cviVDecOpen, %d\n", s32Ret);
            return s32Ret;
        }

        if (vdec_handle->g_stModParam.enVdecVBSource !=
            VB_SOURCE_COMMON) {
            cviVDecAttachCallBack(_cvi_vdec_CallbackFunc);
        }
    }

#if 0
    {
        vdec_dbg *pDbg = &vdecDbg;
        pDbg->currMask = 0x0FFFFFFF;
    }
#endif
    return CVI_SUCCESS;
}

CVI_S32 CVI_VDEC_DestroyChn(VDEC_CHN VdChn)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    struct cvi_vdec_vb_ctx *pVbCtx = NULL;
    vdec_chn_context *pChnHandle = NULL;

    CVI_VDEC_API("\n");

    if (vdec_handle == NULL || vdec_handle->chn_handle[VdChn] == NULL) {
        CVI_VDEC_INFO("VdChn: %d already destoryed.\n", VdChn);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    pVbCtx = pChnHandle->pVbCtx;

    if (pChnHandle->ChnAttr.enType == PT_H264 ||
        pChnHandle->ChnAttr.enType == PT_H265) {
        s32Ret = cviVDecClose(pChnHandle->pHandle);
#ifdef USE_VB_POOL
        if (pChnHandle->bHasVbPool == true) {
            CVI_U32 i = 0;
            VB_BLK blk;

            for (i = 0; i < pChnHandle->FrmNum; i++) {
                blk = vb_phys_addr2handle(
                    pChnHandle->FrmArray[i].phyAddr);
                if (blk != VB_INVALID_HANDLE) {
                    vb_release_block(blk);
                }
            }

            if (vdec_handle->g_stModParam.enVdecVBSource ==
                VB_SOURCE_PRIVATE) {
                vb_destroy_pool(pChnHandle->vbPool.hPicVbPool);
                CVI_VDEC_TRACE("CVI_VB_DestroyPool: %d\n",
                           pChnHandle->vbPool.hPicVbPool);
            }
        }
#endif
    }else {
        CVIJpgReleaseFrameData(pChnHandle->pHandle);
        if (pChnHandle->pHandle != NULL) {
            CVIJpgClose(pChnHandle->pHandle);
            pChnHandle->pHandle = NULL;
        }
        CVIJpgUninit();
    }

    MUTEX_DESTROY(&pChnHandle->display_queue_lock);
    MUTEX_DESTROY(&pChnHandle->status_lock);
    MUTEX_DESTROY(&pChnHandle->chnShmMutex);
    MUTEX_DESTROY(&pChnHandle->jpdLock);

    if (pChnHandle->VideoFrameArray) {
        MEM_FREE(pChnHandle->VideoFrameArray);
        pChnHandle->VideoFrameArray = NULL;
    }

    if (vdec_handle->chn_handle[VdChn]) {
        MEM_FREE(vdec_handle->chn_handle[VdChn]);
        vdec_handle->chn_handle[VdChn] = NULL;
    }

    {
        VDEC_CHN i = 0;
        CVI_BOOL bFreeVdecHandle = CVI_TRUE;

        for (i = 0; i < VDEC_MAX_CHN_NUM; i++) {
            if (vdec_handle->chn_handle[i] != NULL) {
                bFreeVdecHandle = CVI_FALSE;
                break;
            }
        }

    }

    return s32Ret;
}

CVI_S32 CVI_VDEC_SetChnParam(VDEC_CHN VdChn, const VDEC_CHN_PARAM_S *pstParam)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    vdec_chn_context *pChnHandle = NULL;

    CVI_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    memcpy(&pChnHandle->ChnParam, pstParam, sizeof(VDEC_CHN_PARAM_S));
    if ((pChnHandle->ChnAttr.enType == PT_H264) ||  (pChnHandle->ChnAttr.enType == PT_H265)) {
        if (pstParam->enPixelFormat == PIXEL_FORMAT_NV12)
            set_cbcr_format(pChnHandle->pHandle, 1, 0);
        else if (pstParam->enPixelFormat == PIXEL_FORMAT_NV21)
            set_cbcr_format(pChnHandle->pHandle, 1, 1);
        else
            set_cbcr_format(pChnHandle->pHandle, 0, 0);
    }

    return s32Ret;
}

CVI_S32 CVI_VDEC_GetChnParam(VDEC_CHN VdChn, VDEC_CHN_PARAM_S *pstParam)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    vdec_chn_context *pChnHandle = NULL;

    CVI_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    memcpy(pstParam, &pChnHandle->ChnParam, sizeof(VDEC_CHN_PARAM_S));

    return s32Ret;
}

CVI_S32 CVI_VDEC_SendStream(VDEC_CHN VdChn, const VDEC_STREAM_S *pstStream,
                CVI_S32 s32MilliSec)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    CVI_S32 fb_idx;
    VIDEO_FRAME_INFO_S *pstVideoFrame;
    CVI_BOOL bGetDispFrm = CVI_FALSE;
    CVI_BOOL bFlushDecodeQ = CVI_FALSE;
    CVI_S32 s32TimeCost;
    CVI_S32 s32TimeOutMs = s32MilliSec;
    CVI_U64 u64DecHwTime = 0;
    struct cvi_vdec_vb_ctx *pVbCtx = NULL;
    vdec_chn_context *pChnHandle = NULL;

    CVI_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle,chn:%d ret:%d\n", VdChn, s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    pVbCtx = pChnHandle->pVbCtx;

    if ((pstStream->u32Len == 0) &&
        (pChnHandle->ChnAttr.enType == PT_JPEG ||
         pChnHandle->ChnAttr.enType == PT_MJPEG))
        return CVI_SUCCESS;

    if ((pstStream->u32Len == 0) && (pstStream->bEndOfStream == CVI_FALSE))
        return CVI_SUCCESS;

    if ((pstStream->bEndOfStream == CVI_TRUE) &&
        (pChnHandle->ChnAttr.enType != PT_JPEG &&
         pChnHandle->ChnAttr.enType != PT_MJPEG)) {
        bFlushDecodeQ = CVI_TRUE;
    }

    pChnHandle->u32SendStreamCnt++;
    s32Ret = cviSetVdecFpsToProc(VdChn, CVI_TRUE);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("(chn %d) cviSetVdecFpsToProc fail\n", VdChn);
        return s32Ret;
    }

    cviGetDebugConfigFromDecProc();

    while (1) {
        CVI_U64 startTime, endTime;

        bGetDispFrm = CVI_FALSE;

        fb_idx = get_avail_fb(VdChn, pChnHandle->VideoFrameArray);
        if (fb_idx < 0) {
            CVI_VDEC_WARN("cannot get any fb in VideoFrameArray\n");
            return CVI_ERR_VDEC_BUF_FULL;
        }

        pstVideoFrame = &pChnHandle->VideoFrameArray[fb_idx];
        CVI_VDEC_INFO(
            "VdChn = %d, pts %lld, addr %p, len %d, eof %d, eos %d\n",
            VdChn, pstStream->u64PTS, pstStream->pu8Addr,
            pstStream->u32Len, pstStream->bEndOfFrame,
            pstStream->bEndOfStream);

        pChnHandle->stStatus.u32LeftStreamBytes += pstStream->u32Len;

        startTime = get_current_time();

        if (pChnHandle->ChnAttr.enType == PT_JPEG ||
            pChnHandle->ChnAttr.enType == PT_MJPEG) {

            s32Ret = cvi_jpeg_decode(pChnHandle, pstStream, pstVideoFrame, s32TimeOutMs, &u64DecHwTime);
            if (s32Ret != CVI_SUCCESS) {
                if (s32Ret == CVI_ERR_VDEC_BUSY) {
                    pChnHandle->stStatus.u32LeftStreamBytes -= pstStream->u32Len;
                    CVI_VDEC_TRACE(
                        "jpeg timeout in nonblock mode[%d]\n",
                        s32TimeOutMs);
                    return s32Ret;
                }
                CVI_VDEC_ERR("cvi_jpeg_decode error\n");
                goto ERR_CVI_VDEC_SEND_STREAM;
            }

            s32Ret = cviSetVideoChnAttrToProc(VdChn, pstVideoFrame,
                              u64DecHwTime);
            if (s32Ret != CVI_SUCCESS) {
                CVI_VDEC_ERR("cviSetVideoChnAttrToProc fail");
                return s32Ret;
            }

            s32Ret = cviVdec_Mutex_Lock(
                &pChnHandle->display_queue_lock,
                VDEC_TIME_BLOCK_MODE, &s32TimeCost);
            s32Ret = checkTimeOutAndBusy(s32Ret, __LINE__);
            if (s32Ret != 0) {
                return s32Ret;
            }
            pstVideoFrame->stVFrame.u32FrameFlag = 1;
            pstVideoFrame->stVFrame.u64PTS = pstStream->u64PTS;
            s32Ret =
                insert_display_queue(pChnHandle->w_idx, fb_idx,
                             pChnHandle->display_queue);
            cviVdec_Mutex_Unlock(&pChnHandle->display_queue_lock);
            if (s32Ret != CVI_SUCCESS) {
                CVI_VDEC_ERR(
                    "insert_display_queue fail, w_idx %d, fb_idx %d\n",
                    pChnHandle->w_idx, fb_idx);
                return s32Ret;
            }

            pChnHandle->w_idx++;
            if (pChnHandle->w_idx == DISPLAY_QUEUE_SIZE)
                pChnHandle->w_idx = 0;
            s32Ret = cviVdec_Mutex_Lock(&pChnHandle->status_lock,
                            VDEC_TIME_BLOCK_MODE,
                            &s32TimeCost);
            s32Ret = checkTimeOutAndBusy(s32Ret, __LINE__);
            if (s32Ret != 0) {
                return s32Ret;
            }
            pChnHandle->stStatus.u32LeftPics++;
            wake_up(&tVdecWaitQueue[VdChn]);
            cviVdec_Mutex_Unlock(&pChnHandle->status_lock);
        } else if ((pChnHandle->ChnAttr.enType == PT_H264) ||
               (pChnHandle->ChnAttr.enType == PT_H265)) {
            if (pChnHandle->ChnAttr.enType == PT_H264) {
                s32Ret = cvi_h264_decode(
                    pChnHandle->pHandle,
                    pChnHandle->ChnParam.enPixelFormat,
                    pstStream, s32TimeOutMs);
            } else {
                s32Ret = cvi_h265_decode(
                    pChnHandle->pHandle,
                    pChnHandle->ChnParam.enPixelFormat,
                    pstStream, s32TimeOutMs);
            }

            if (s32Ret == RETCODE_SUCCESS) {
                s32Ret = CVI_SUCCESS;
                wake_up(&tVdecWaitQueue[VdChn]);
                pChnHandle->stStatus.u32LeftStreamBytes -= pstStream->u32Len;
                break;
            } if (s32Ret == RETCODE_QUEUEING_FAILURE) {
                s32Ret = CVI_ERR_VDEC_BUF_FULL;
                pChnHandle->stStatus.u32LeftStreamBytes -= pstStream->u32Len;
                goto ERR_CVI_VDEC_SEND_STREAM;
            }

        } else {
            CVI_VDEC_ERR("enType = %d\n", pChnHandle->ChnAttr.enType);
            s32Ret = CVI_ERR_VDEC_NOT_SUPPORT;
            goto ERR_CVI_VDEC_SEND_STREAM;
        }

        pChnHandle->stStatus.u32LeftStreamBytes -= pstStream->u32Len;

        endTime = get_current_time();

        pChnHandle->totalDecTime += (endTime - startTime);
        CVI_VDEC_PERF(
            "SendStream timestamp = %llu , dec time = %llu ms, total = %llu ms\n",
            (unsigned long long)(startTime),
            (unsigned long long)(endTime - startTime),
            (unsigned long long)(pChnHandle->totalDecTime));

        if ((pstStream->bEndOfStream == CVI_TRUE) &&
            (s32MilliSec > 0)) {
            CVI_VDEC_TRACE(
                "force flush in EndOfStream in nonblock mode flush dec\n");
            s32TimeOutMs = -1;
        }

        if (s32Ret == CVI_SUCCESS)
            break;
    }

ERR_CVI_VDEC_SEND_STREAM:
    // set_current_state(TASK_INTERRUPTIBLE);
    // schedule_timeout(usecs_to_jiffies(1000));
    return s32Ret;
}



CVI_S32 CVI_VDEC_QueryStatus(VDEC_CHN VdChn, VDEC_CHN_STATUS_S *pstStatus)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    vdec_chn_context *pChnHandle = NULL;

    CVI_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    memcpy(pstStatus, &pChnHandle->stStatus, sizeof(VDEC_CHN_STATUS_S));
    if(pChnHandle->ChnAttr.enType == PT_H264
        ||pChnHandle->ChnAttr.enType == PT_H265)
    get_status(pChnHandle->pHandle, pstStatus);
    return s32Ret;
}

CVI_S32 CVI_VDEC_ResetChn(VDEC_CHN VdChn)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    VDEC_CHN_ATTR_S *pstChnAttr;
    CVI_U32 u32FrameBufCnt;
    vdec_chn_context *pChnHandle = NULL;

    CVI_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    pstChnAttr = &pChnHandle->ChnAttr;

    if (pstChnAttr->enType == PT_H264 || pstChnAttr->enType == PT_H265) {
        s32Ret = cviVDecReset(pChnHandle->pHandle);
        if (s32Ret < 0) {
            CVI_VDEC_ERR("cviVDecReset, %d\n", s32Ret);
            return CVI_ERR_VDEC_ERR_INVALID_RET;
        }

        if (pChnHandle->VideoFrameArray != NULL) {
            u32FrameBufCnt = pChnHandle->ChnAttr.u32FrameBufCnt;
            memset(pChnHandle->VideoFrameArray, 0,
                   sizeof(VIDEO_FRAME_INFO_S) * u32FrameBufCnt);
        }
    }

    return s32Ret;
}

CVI_S32 CVI_VDEC_StartRecvStream(VDEC_CHN VdChn)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    vdec_chn_context *pChnHandle = NULL;

    CVI_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    if (pChnHandle->bStartRecv == CVI_FALSE) {
        CVI_U32 i = 0;

        //create VB buffer
        for (i = 0; i < pChnHandle->ChnAttr.u32FrameBufCnt; i++) {
            s32Ret = allocate_vdec_frame(pChnHandle, i);
            if (s32Ret != CVI_SUCCESS) {
                CVI_VDEC_ERR("allocate_vdec_frame, %d\n",
                         s32Ret);
                return s32Ret;
            }
        }
    }

    pChnHandle->bStartRecv = CVI_TRUE;
    return s32Ret;
}

CVI_S32 CVI_VDEC_StopRecvStream(VDEC_CHN VdChn)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    vdec_chn_context *pChnHandle = NULL;

    CVI_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    if (pChnHandle->bStartRecv == CVI_TRUE) {
        if (pChnHandle->ChnAttr.enType == PT_JPEG ||
            pChnHandle->ChnAttr.enType == PT_MJPEG) {
            CVI_U32 i = 0;

            for (i = 0; i < pChnHandle->ChnAttr.u32FrameBufCnt;
                 i++) {
                s32Ret = free_vdec_frame(
                    &pChnHandle->ChnAttr,
                    &pChnHandle->VideoFrameArray[i]);
                if (s32Ret != CVI_SUCCESS) {
                    CVI_VDEC_ERR("free_vdec_frame, %ud\n",
                             s32Ret);
                    return s32Ret;
                }
            }
        }
    }

    pChnHandle->bStartRecv = CVI_FALSE;
    return s32Ret;
}

CVI_S32 CVI_VDEC_GetFrame(VDEC_CHN VdChn, VIDEO_FRAME_INFO_S *pstFrameInfo,
              CVI_S32 s32MilliSec)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    CVI_S32 s32TimeCost;
    vdec_chn_context *pChnHandle = NULL;
    CVI_S32 fb_idx;

    CVI_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    if (pChnHandle->ChnAttr.enType == PT_JPEG
        || pChnHandle->ChnAttr.enType == PT_MJPEG) {
        s32Ret = cviVdec_Mutex_Lock(&pChnHandle->display_queue_lock,
                        s32MilliSec, &s32TimeCost);
        s32Ret = checkTimeOutAndBusy(s32Ret, __LINE__);
        if (s32Ret != 0) {
            return s32Ret;
        }
        if (pChnHandle->stStatus.u32LeftPics <= 0) {
            CVI_VDEC_ERR("u32LeftPics %d\n",
                     pChnHandle->stStatus.u32LeftPics);
            cviVdec_Mutex_Unlock(&pChnHandle->display_queue_lock);
            return CVI_ERR_VDEC_BUF_EMPTY;
        }

        fb_idx = pChnHandle->display_queue[pChnHandle->r_idx];
        cviVdec_Mutex_Unlock(&pChnHandle->display_queue_lock);
        if (fb_idx < 0) {
            CVI_VDEC_WARN("get display queue fail, r_idx %d, fb_idx %d\n",
                      pChnHandle->r_idx, fb_idx);
            return CVI_ERR_VDEC_ERR_INVALID_RET;
        }

        pChnHandle->u32GetFrameCnt++;
        s32Ret = cviSetVdecFpsToProc(VdChn, CVI_FALSE);
        if (s32Ret != CVI_SUCCESS) {
            CVI_VDEC_ERR("(chn %d) cviSetVdecFpsToProc fail\n", VdChn);
            return s32Ret;
        }

        memcpy(pstFrameInfo, &pChnHandle->VideoFrameArray[fb_idx],
               sizeof(VIDEO_FRAME_INFO_S));

        s32Ret = cviVdec_Mutex_Lock(&pChnHandle->status_lock,
                        VDEC_TIME_BLOCK_MODE, &s32TimeCost);
        s32Ret = checkTimeOutAndBusy(s32Ret, __LINE__);
        if (s32Ret != 0) {
            return s32Ret;
        }
        pChnHandle->stStatus.u32LeftPics--;
        pstFrameInfo->stVFrame.u32TimeRef = pChnHandle->seqNum;
        pChnHandle->seqNum += 2;
        pChnHandle->display_queue[pChnHandle->r_idx] = -1;
        pChnHandle->r_idx++;
        if (pChnHandle->r_idx == DISPLAY_QUEUE_SIZE)
            pChnHandle->r_idx = 0;
        cviVdec_Mutex_Unlock(&pChnHandle->status_lock);
    }else {
        cviDispFrameCfg dfc = {0};
        s32Ret = cviVDecGetFrame(pChnHandle->pHandle, &dfc);
        if (s32Ret >= 0) {
            cviSetVideoFrameInfo(pstFrameInfo, &dfc);
        }

        pChnHandle->stFPS.u64HwTime = dfc.decHwTime;
    }
    return s32Ret;
}

CVI_S32 CVI_VDEC_ReleaseFrame(VDEC_CHN VdChn,
                  const VIDEO_FRAME_INFO_S *pstFrameInfo)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    vdec_chn_context *pChnHandle;
    CVI_S32 fb_idx;
    VIDEO_FRAME_S *pstVFrame;

    CVI_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    if (pChnHandle->ChnAttr.enType == PT_JPEG
        ||pChnHandle->ChnAttr.enType == PT_MJPEG) {
        s32Ret = cviVdec_Mutex_Lock(&pChnHandle->display_queue_lock,
                VDEC_DEFAULT_MUTEX_MODE, NULL);
        s32Ret = checkTimeOutAndBusy(s32Ret, __LINE__);
        if (s32Ret != 0) {
            return s32Ret;
        }
        fb_idx = _cvi_vdec_FindFrameIdx(pChnHandle, pstFrameInfo);
        if (fb_idx < 0) {
            CVI_VDEC_ERR("Can't find video frame in VideoFrameArray!\n");
            cviVdec_Mutex_Unlock(&pChnHandle->display_queue_lock);
            return fb_idx;
        }
        pstVFrame = &pChnHandle->VideoFrameArray[fb_idx].stVFrame;
        CVIJpgReleaseFrameData(pChnHandle->pHandle);
        if (pChnHandle->pHandle != NULL) {
            CVIJpgClose(pChnHandle->pHandle);
            pChnHandle->pHandle = NULL;
        }
        pstVFrame->u32FrameFlag = 0;
        cviVdec_Mutex_Unlock(&pChnHandle->display_queue_lock);
    }

    if ((pChnHandle->ChnAttr.enType == PT_H264) ||
        (pChnHandle->ChnAttr.enType == PT_H265)) {
        pstVFrame = (VIDEO_FRAME_S *)&pstFrameInfo->stVFrame;
        cviVDecReleaseFrame(pChnHandle->pHandle,
                    pstVFrame->pPrivateData, pstVFrame->u64PhyAddr[0]);
    }

    CVI_VDEC_INFO("release fb_idx %d\n", fb_idx);
    return CVI_SUCCESS;
}

CVI_S32 CVI_VDEC_SetChnAttr(VDEC_CHN VdChn, const VDEC_CHN_ATTR_S *pstAttr)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    vdec_chn_context *pChnHandle = NULL;

    CVI_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    memcpy(&pChnHandle->ChnAttr, pstAttr, sizeof(VDEC_CHN_ATTR_S));

    return s32Ret;
}

CVI_S32 CVI_VDEC_GetChnAttr(VDEC_CHN VdChn, VDEC_CHN_ATTR_S *pstAttr)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    vdec_chn_context *pChnHandle = NULL;

    CVI_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    memcpy(pstAttr, &pChnHandle->ChnAttr, sizeof(VDEC_CHN_ATTR_S));

    return s32Ret;
}

CVI_S32 CVI_VDEC_AttachVbPool(VDEC_CHN VdChn, const VDEC_CHN_POOL_S *pstPool)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    vdec_chn_context *pChnHandle = NULL;

    CVI_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstPool == NULL) {
        return CVI_ERR_VDEC_NULL_PTR;
    }

    if (vdec_handle->g_stModParam.enVdecVBSource != VB_SOURCE_USER) {
        CVI_VDEC_ERR("Not support enVdecVBSource:%d\n",
                 vdec_handle->g_stModParam.enVdecVBSource);
        return CVI_ERR_VDEC_NOT_SUPPORT;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];

    pChnHandle->vbPool = *pstPool;
    pChnHandle->bHasVbPool = true;
    if (pChnHandle->ChnAttr.enType == PT_H264 || pChnHandle->ChnAttr.enType == PT_H265) {
        VB_INFO vb_info;
        vb_info.frame_buffer_vb_pool = pstPool->hPicVbPool;
        vb_info.vb_mode = vdec_handle->g_stModParam.enVdecVBSource;
        cviVDecAttachFrmBuf(pChnHandle->pHandle, vb_info);
    }

    return s32Ret;
}

CVI_S32 CVI_VDEC_DetachVbPool(VDEC_CHN VdChn)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    vdec_chn_context *pChnHandle = NULL;

    CVI_VDEC_API("\n");

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (vdec_handle->g_stModParam.enVdecVBSource != VB_SOURCE_USER) {
        CVI_VDEC_ERR("Invalid detachVb in ChnId[%d] VBSource:[%d]\n",
                 VdChn, vdec_handle->g_stModParam.enVdecVBSource);
        return CVI_ERR_VDEC_NOT_SUPPORT;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    if (pChnHandle->bStartRecv != CVI_FALSE) {
        CVI_VDEC_ERR("Cannot detach vdec vb before StopRecvStream\n");
        return CVI_ERR_VDEC_ERR_SEQ_OPER;
    }

    if (pChnHandle->bHasVbPool == false) {
        CVI_VDEC_ERR("ChnId[%d] Null VB\n", VdChn);
        return CVI_SUCCESS;

    } else {
        #ifdef USE_VB_POOL
        if (pChnHandle->ChnAttr.enType == PT_H264 ||
            pChnHandle->ChnAttr.enType == PT_H265) {
            CVI_VDEC_API("26x detach\n");
            if (pChnHandle->bHasVbPool == true) {
                CVI_U32 i = 0;
                VB_BLK blk;

                for (i = 0; i < pChnHandle->FrmNum; i++) {
                    blk = vb_phys_addr2handle(
                        pChnHandle->FrmArray[i].phyAddr);

                    if (blk != VB_INVALID_HANDLE)
                        vb_release_block(blk);
                }
            }
        }
        #endif
        pChnHandle->bHasVbPool = false;
    }

    return CVI_SUCCESS;
}

CVI_S32 CVI_VDEC_SetModParam(const VDEC_MOD_PARAM_S *pstModParam)
{
    CVI_S32 s32Ret = CVI_SUCCESS;

    if (pstModParam == NULL) {
        return CVI_ERR_VDEC_ILLEGAL_PARAM;
    }

    s32Ret = cviVdec_Mutex_Lock(&g_vdec_handle_mutex,
                    VDEC_DEFAULT_MUTEX_MODE, NULL);
    s32Ret = checkTimeOutAndBusy(s32Ret, __LINE__);
    if (s32Ret != 0) {
        return s32Ret;
    }
    memcpy(&vdec_handle->g_stModParam, pstModParam,
           sizeof(VDEC_MOD_PARAM_S));
    cviVdec_Mutex_Unlock(&g_vdec_handle_mutex);

    return s32Ret;
}

CVI_S32 CVI_VDEC_GetModParam(VDEC_MOD_PARAM_S *pstModParam)
{
    CVI_S32 s32Ret = CVI_SUCCESS;

    if (pstModParam == NULL) {
        return CVI_ERR_VDEC_ILLEGAL_PARAM;
    }

    s32Ret = cviVdec_Mutex_Lock(&g_vdec_handle_mutex,
                    VDEC_DEFAULT_MUTEX_MODE, NULL);
    if (s32Ret != 0) {
        if ((s32Ret == ETIMEDOUT) || (s32Ret == EBUSY)) {
            CVI_VDEC_TRACE("mutex timeout and retry\n");
            return  CVI_ERR_VDEC_BUSY;
        }
        CVI_VDEC_ERR("vdec mutex error[%d]\n", s32Ret);
        return CVI_ERR_VDEC_ERR_VDEC_MUTEX;
    }
    memcpy(pstModParam, &vdec_handle->g_stModParam,
           sizeof(VDEC_MOD_PARAM_S));
    cviVdec_Mutex_Unlock(&g_vdec_handle_mutex);

    return s32Ret;
}

CVI_S32 CVI_VDEC_FrameBufferAddUser(VDEC_CHN VdChn, VIDEO_FRAME_INFO_S *pstFrameInfo)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    vdec_chn_context *pChnHandle;

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    return frame_bufer_add_user(pChnHandle->pHandle, (uintptr_t)pstFrameInfo->stVFrame.pPrivateData);
}

CVI_S32 drv_vdec_set_stride_align(VDEC_CHN VdChn, CVI_U32 align)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    vdec_chn_context *pChnHandle;

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    return set_stride_align(pChnHandle->pHandle, align);
}

CVI_S32 drv_vdec_set_user_pic(VDEC_CHN VdChn, const VIDEO_FRAME_INFO_S *usr_pic)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    vdec_chn_context *pChnHandle;
    cviDispFrameCfg frame_cfg = {0};

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    frame_cfg.width = usr_pic->stVFrame.u32Width;
    frame_cfg.height = usr_pic->stVFrame.u32Height;
    frame_cfg.strideY = usr_pic->stVFrame.u32Stride[0];
    frame_cfg.strideC = usr_pic->stVFrame.u32Stride[1];
    frame_cfg.indexFrameDisplay = -1;

    if (usr_pic->stVFrame.enPixelFormat == PIXEL_FORMAT_NV12) {
        frame_cfg.cbcrInterleave = 1;
        frame_cfg.nv21 = 0;
    } else if (usr_pic->stVFrame.enPixelFormat == PIXEL_FORMAT_NV21) {
        frame_cfg.cbcrInterleave = 1;
        frame_cfg.nv21 = 1;
    } else {//PIXEL_FORMAT_YUV_PLANAR_420
        frame_cfg.cbcrInterleave = 0;
        frame_cfg.nv21 = 0;
    }

    frame_cfg.phyAddr[0] = usr_pic->stVFrame.u64PhyAddr[0];
    frame_cfg.phyAddr[1] = usr_pic->stVFrame.u64PhyAddr[1];
    frame_cfg.phyAddr[2] = usr_pic->stVFrame.u64PhyAddr[2];
    frame_cfg.addr[0] = usr_pic->stVFrame.pu8VirAddr[0];
    frame_cfg.addr[1] = usr_pic->stVFrame.pu8VirAddr[1];
    frame_cfg.addr[2] = usr_pic->stVFrame.pu8VirAddr[2];
    frame_cfg.length[0] = usr_pic->stVFrame.u32Length[0];
    frame_cfg.length[1] = usr_pic->stVFrame.u32Length[1];
    frame_cfg.length[2] = usr_pic->stVFrame.u32Length[2];
    frame_cfg.bCompressFrame = 0;

    frame_cfg.endian = usr_pic->stVFrame.u8Endian;
    frame_cfg.picType = usr_pic->stVFrame.u8PicType;
    frame_cfg.seqenceNo = usr_pic->stVFrame.u32SeqenceNo;
    frame_cfg.interlacedFrame = usr_pic->stVFrame.u8InterlacedFrame;
    frame_cfg.decHwTime = 0;
    return set_user_pic(pChnHandle->pHandle, &frame_cfg);

}

CVI_S32 drv_vdec_enable_user_pic(VDEC_CHN VdChn, CVI_BOOL instant)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    vdec_chn_context *pChnHandle;

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    return enable_user_pic(pChnHandle->pHandle, instant);
}

CVI_S32 drv_vdec_disable_user_pic(VDEC_CHN VdChn)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    vdec_chn_context *pChnHandle;

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    return disable_user_pic(pChnHandle->pHandle);

}

CVI_S32 drv_vdec_set_display_mode(VDEC_CHN VdChn, VIDEO_DISPLAY_MODE_E display_mode)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    vdec_chn_context *pChnHandle;

    s32Ret = check_vdec_chn_handle(VdChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VDEC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = vdec_handle->chn_handle[VdChn];
    return set_display_mode(pChnHandle->pHandle, display_mode);
}

#endif
