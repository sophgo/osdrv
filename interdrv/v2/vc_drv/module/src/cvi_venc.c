#include "cvi_venc.h"
#include <linux/cvi_comm_venc.h>
#include <linux/cvi_defines.h>
#include "venc.h"
#include "bind.h"
#include "module_common.h"
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <uapi/linux/sched/types.h>

#include "cvi_venc_rc.h"


venc_context *handle;

extern wait_queue_head_t tVencWaitQueue[];

#ifdef USE_VB_POOL
extern int32_t vb_create_pool(struct cvi_vb_pool_cfg *config);
extern int32_t vb_get_config(struct cvi_vb_cfg *pstVbConfig);
extern VB_BLK vb_get_block_with_id(VB_POOL poolId, uint32_t u32BlkSize,
                   MOD_ID_E modId);
extern uint64_t vb_handle2phys_addr(VB_BLK blk);
extern VB_BLK vb_phys_addr2handle(uint64_t u64PhyAddr);
extern int32_t vb_release_block(VB_BLK blk);
#endif

static CVI_S32 cviProcessResult(venc_chn_context *pChnHandle,
                        VENC_STREAM_S *pstStream);

venc_vb_ctx	vencVbCtx[VENC_MAX_CHN_NUM];

static inline CVI_U32 _cviGetNumPacks(PAYLOAD_TYPE_E enType)
{
    return (enType == PT_JPEG || enType == PT_MJPEG) ? 2 : MAX_NUM_PACKS;
}

static inline CVI_S32 cviCheckCommonRCParamHelper(
    CVI_U32 u32MaxIprop, CVI_U32 u32MinIprop,
    CVI_U32 u32MaxIQp, CVI_U32 u32MinIQp,
    CVI_U32 u32MaxQp, CVI_U32 u32MinQp)
{
    CVI_S32 s32Ret = CVI_SUCCESS;

    s32Ret = ((u32MaxIprop >= CVI_H26X_MAX_I_PROP_MIN) &&
        (u32MaxIprop <= CVI_H26X_MAX_I_PROP_MAX)) ?
        CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("error, %s = %d\n",
            "u32MaxIprop", u32MaxIprop);
        return s32Ret;
    }

    s32Ret = ((u32MinIprop >= CVI_H26X_MIN_I_PROP_MIN) &&
        (u32MinIprop <= CVI_H26X_MAX_I_PROP_MAX)) ?
        CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("error, %s = %d\n",
            "u32MinIprop", u32MinIprop);
        return s32Ret;
    }

    s32Ret = (u32MaxIQp <= 51) ?
        CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("error, %s = %d\n",
            "u32MaxIQp", u32MaxIQp);
        return s32Ret;
    }

    s32Ret = (u32MinIQp <= 51) ?
        CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("error, %s = %d\n",
            "u32MinIQp", u32MinIQp);
        return s32Ret;
    }

    s32Ret = (u32MaxQp <= 51) ?
        CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("error, %s = %d\n",
            "u32MaxQp", u32MaxQp);
        return s32Ret;
    }

    s32Ret = (u32MinQp <= 51) ?
        CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("error, %s = %d\n",
            "u32MinQp", u32MinQp);
        return s32Ret;
    }

    return s32Ret;
}

static inline CVI_S32 cviCheckCommonRCParam(
    const VENC_RC_PARAM_S * pstRcParam, VENC_ATTR_S *pVencAttr, VENC_RC_MODE_E enRcMode)
{
    CVI_S32 s32Ret;

    s32Ret = (pstRcParam->u32RowQpDelta <= CVI_H26X_ROW_QP_DELTA_MAX) ?
        CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("error, %s = %d\n",
            "u32RowQpDelta", pstRcParam->u32RowQpDelta);
        return s32Ret;
    }

    if (pVencAttr->enType != PT_H265 ||
        pstRcParam->s32FirstFrameStartQp != 63) {
        s32Ret = ((pstRcParam->s32FirstFrameStartQp >= 0) &&
            (pstRcParam->s32FirstFrameStartQp <= 51)) ? CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("error, %s = %d\n",
                "s32FirstFrameStartQp", pstRcParam->s32FirstFrameStartQp);
            return s32Ret;
        }
    }

    s32Ret = (pstRcParam->u32ThrdLv <= CVI_H26X_THRDLV_MAX) ?
        CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("error, %s = %d\n",
            "u32ThrdLv", pstRcParam->u32ThrdLv);
        return s32Ret;
    }

    s32Ret = ((pstRcParam->s32BgDeltaQp >= CVI_H26X_BG_DELTA_QP_MIN) &&
        (pstRcParam->s32BgDeltaQp <= CVI_H26X_BG_DELTA_QP_MAX)) ?
        CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("error, %s = %d\n",
            "s32BgDeltaQp", pstRcParam->s32BgDeltaQp);
        return s32Ret;
    }

    switch (enRcMode) {
    case VENC_RC_MODE_H264CBR:
        {
            const VENC_PARAM_H264_CBR_S *pprc = &pstRcParam->stParamH264Cbr;

            s32Ret = cviCheckCommonRCParamHelper(pprc->u32MaxIprop, pprc->u32MinIprop,
                pprc->u32MaxIQp, pprc->u32MinIQp, pprc->u32MaxQp, pprc->u32MinQp);
        }
        break;
    case VENC_RC_MODE_H264VBR:
        {
            const VENC_PARAM_H264_VBR_S *pprc = &pstRcParam->stParamH264Vbr;

            s32Ret = cviCheckCommonRCParamHelper(pprc->u32MaxIprop, pprc->u32MinIprop,
                pprc->u32MaxIQp, pprc->u32MinIQp, pprc->u32MaxQp, pprc->u32MinQp);
        }
        break;
    case VENC_RC_MODE_H264AVBR:
        {
            const VENC_PARAM_H264_AVBR_S *pprc = &pstRcParam->stParamH264AVbr;

            s32Ret = cviCheckCommonRCParamHelper(pprc->u32MaxIprop, pprc->u32MinIprop,
                pprc->u32MaxIQp, pprc->u32MinIQp, pprc->u32MaxQp, pprc->u32MinQp);
        }
        break;
    case VENC_RC_MODE_H264UBR:
        {
            const VENC_PARAM_H264_UBR_S *pprc = &pstRcParam->stParamH264Ubr;

            s32Ret = cviCheckCommonRCParamHelper(pprc->u32MaxIprop, pprc->u32MinIprop,
                pprc->u32MaxIQp, pprc->u32MinIQp, pprc->u32MaxQp, pprc->u32MinQp);
        }
        break;
    case VENC_RC_MODE_H265CBR:
        {
            const VENC_PARAM_H265_CBR_S *pprc = &pstRcParam->stParamH265Cbr;

            s32Ret = cviCheckCommonRCParamHelper(pprc->u32MaxIprop, pprc->u32MinIprop,
                pprc->u32MaxIQp, pprc->u32MinIQp, pprc->u32MaxQp, pprc->u32MinQp);
        }
        break;
    case VENC_RC_MODE_H265VBR:
        {
            const VENC_PARAM_H265_VBR_S *pprc = &pstRcParam->stParamH265Vbr;

            s32Ret = cviCheckCommonRCParamHelper(pprc->u32MaxIprop, pprc->u32MinIprop,
                pprc->u32MaxIQp, pprc->u32MinIQp, pprc->u32MaxQp, pprc->u32MinQp);
        }
        break;
    case VENC_RC_MODE_H265AVBR:
        {
            const VENC_PARAM_H265_AVBR_S *pprc = &pstRcParam->stParamH265AVbr;

            s32Ret = cviCheckCommonRCParamHelper(pprc->u32MaxIprop, pprc->u32MinIprop,
                pprc->u32MaxIQp, pprc->u32MinIQp, pprc->u32MaxQp, pprc->u32MinQp);
        }
        break;
    case VENC_RC_MODE_H265UBR:
        {
            const VENC_PARAM_H265_UBR_S *pprc = &pstRcParam->stParamH265Ubr;

            s32Ret = cviCheckCommonRCParamHelper(pprc->u32MaxIprop, pprc->u32MinIprop,
                pprc->u32MaxIQp, pprc->u32MinIQp, pprc->u32MaxQp, pprc->u32MinQp);
        }
        break;
    default:
        break;
    }

    return s32Ret;
}

void *venc_get_share_mem(void)
{
    return NULL;
}

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


static int venc_recv_qbuf(MMF_CHN_S chn, VB_BLK blk)
{
    CVI_S32 ret;
    VENC_CHN VencChn = chn.s32ChnId;

    // sanity check
    if (VencChn >= VC_MAX_CHN_NUM) {
        CVI_VENC_ERR("invalid venc chn\n");
        return CVI_FAILURE;
    }

    ret = vb_qbuf(chn, CHN_TYPE_IN, &vencVbCtx[VencChn].vb_jobs, blk);

    return ret;
}

static CVI_S32 cviWaitEncodeDone(venc_chn_context *pChnHandle)
{
    CVI_S32 ret = CVI_SUCCESS;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }

    if (pEncCtx->base.ioctl) {
        if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H265 ||
            pChnHandle->pChnAttr->stVencAttr.enType == PT_H264) {
            ret = pEncCtx->base.ioctl(pEncCtx,
                CVI_H26X_OP_WAIT_FRAME_DONE, NULL);
            if (ret != CVI_SUCCESS) {
                MUTEX_UNLOCK(&pChnHandle->chnMutex);
                CVI_VENC_ERR("CVI_H26X_OP_WAIT_FRAME_DONE, %d\n", ret);
                return -1;
            }
        } else {
            // for jpeg, always return success
            ret = CVI_SUCCESS;
        }
    }
    MUTEX_UNLOCK(&pChnHandle->chnMutex);
    if (ret != CVI_SUCCESS)
        CVI_VENC_ERR("failed to wait done %d", ret);

    return ret;
}

static int venc_event_handler(CVI_VOID *data)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)data;
    VENC_CHN VencChn;
    venc_chn_vars *pChnVars = NULL;
    VENC_CHN_STATUS_S stStat;
    venc_enc_ctx *pEncCtx;
    CVI_S32 ret;
    CVI_S32 s32MilliSec = -1;
    MMF_CHN_S chn = { .enModId = CVI_ID_VENC,
                .s32DevId = 0,
                .s32ChnId = 0 };
    VB_BLK blk;
    struct vb_s *vb;
    VIDEO_FRAME_INFO_S stVFrame;
    VIDEO_FRAME_S *pstVFrame = &stVFrame.stVFrame;
    int i;
    int vi_cnt = 0;
    CVI_BOOL bIsoSendFrmEn;
#ifdef DUMP_BIND_YUV
    FILE *out_f = fopen("out.265", "wb");
#endif
    CVI_S32 s32SetFrameMilliSec = 20000;
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_vb_ctx *pVbCtx = NULL;

    memset(&stVFrame, 0, sizeof(VIDEO_FRAME_INFO_S));

    pChnVars = pChnHandle->pChnVars;
    pEncCtx = &pChnHandle->encCtx;
    pVbCtx = pChnHandle->pVbCtx;
    VencChn = pChnHandle->VeChn;

    chn.s32ChnId = VencChn;
    bIsoSendFrmEn = pChnHandle->pChnAttr->stVencAttr.bIsoSendFrmEn;
    CVI_VENC_DBG("venc_event_handler bIsoSendFrmEn:%d\n", bIsoSendFrmEn);
    memset(&stVFrame, 0, sizeof(VIDEO_FRAME_INFO_S));

    if (SEMA_WAIT(&pChnVars->sem_send) != 0) {
        CVI_VENC_ERR("can not down sem_send\n");
        return CVI_FAILURE;
    }
    pstVFrame->u32Width = pChnHandle->pChnAttr->stVencAttr.u32PicWidth;
    pstVFrame->u32Height = pChnHandle->pChnAttr->stVencAttr.u32PicHeight;
    CVI_VENC_DBG("[%d] VENC_CHN_STATE_START_ENC\n", VencChn);

    while (!kthread_should_stop() && pChnHandle->bChnEnable) {
        CVI_VENC_DBG("[%d]\n", VencChn);
        // if (IF_WANNA_DISABLE_BIND_MODE() ||
        //     (pChnVars->s32RecvPicNum > 0 &&
        //      vi_cnt >= pChnVars->s32RecvPicNum)) {
        // 	pChnHandle->bChnEnable = CVI_FALSE;
        // 	CVI_VENC_SYNC("end\n");
        // 	break;
        // }

        CVI_VENC_DBG("h26x_handle chn:%d wait.\n", VencChn);

        while (((ret = SEMA_TIMEWAIT(&pVbCtx->vb_jobs.sem, usecs_to_jiffies(1000 * 1000))) != 0)) {
            if (pChnHandle->bChnEnable == CVI_FALSE)
                break;

            continue;
        }
        if (pChnHandle->bChnEnable == CVI_FALSE) {
            CVI_VENC_DBG("end\n");
            break;
        }

        if (ret == -1)
            continue;

        if (pVbCtx->pause) {
            CVI_VENC_DBG("pause and skip update.\n");
            continue;
        }

        // get venc input buf.
        CVI_VENC_DBG("h26x_handle wait jobs chn:%d\n", VencChn);
        blk = base_mod_jobs_waitq_pop(&pVbCtx->vb_jobs);
        if (blk == VB_INVALID_HANDLE) {
            CVI_VENC_DBG("No more vb for dequeue.\n");
            continue;
        }
        vb = (struct vb_s *)blk;

        atomic_long_fetch_and(~BIT(CVI_ID_VENC), &vb->mod_ids);
        for (i = 0; i < 3; i++) {
            pstVFrame->u64PhyAddr[i] = vb->buf.phy_addr[i];
            pstVFrame->u32Stride[i] = vb->buf.stride[i];
            CVI_VENC_DBG("phy: 0x%llx, stride: 0x%x\n",
                        pstVFrame->u64PhyAddr[i],
                        pstVFrame->u32Stride[i]);
        }
        pstVFrame->u64PTS = vb->buf.u64PTS;
        pstVFrame->enPixelFormat = vb->buf.enPixelFormat;
        pstVFrame->pPrivateData = vb;
        CVI_VENC_DBG("h26x_handle chn:%d send.\n", VencChn);
        s32SetFrameMilliSec = 20000;

        CVI_VENC_DBG("[%d]\n", VencChn);
        s32Ret = CVI_VENC_SendFrame(VencChn, &stVFrame,
                        s32SetFrameMilliSec);
        if (s32Ret == CVI_ERR_VENC_INIT ||
            s32Ret == CVI_ERR_VENC_FRC_NO_ENC ||
            s32Ret == CVI_ERR_VENC_BUSY) {
            CVI_VENC_INFO("no encode,continue\n");
            vb_done_handler(chn, CHN_TYPE_IN, &pVbCtx->vb_jobs, blk);
            continue;
        } else if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR(
                "CVI_VENC_SendFrame, VencChn = %d, s32Ret = %d\n",
                VencChn, s32Ret);
            goto VENC_EVENT_HANDLER_ERR;
        }

        // check bIsoSendFrmEn for sync getstream or async getstream
        if (bIsoSendFrmEn == CVI_FALSE) {
            s32Ret = CVI_VENC_QueryStatus(VencChn, &stStat);
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR(
                    "CVI_VENC_QueryStatus, VencChn = %d, s32Ret = %d\n",
                    VencChn, s32Ret);
                goto VENC_EVENT_HANDLER_ERR;
            }
            if (!stStat.u32CurPacks) {
                CVI_VENC_ERR("u32CurPacks = 0\n");
                s32Ret = CVI_ERR_VENC_EMPTY_PACK;
                goto VENC_EVENT_HANDLER_ERR;
            }
            pChnVars->stStream.pstPack = (VENC_PACK_S *)MEM_MALLOC(
                sizeof(VENC_PACK_S) * stStat.u32CurPacks);
            if (pChnVars->stStream.pstPack == NULL) {
                CVI_VENC_ERR("malloc memory failed!\n");
                s32Ret = CVI_ERR_VENC_NOMEM;
                goto VENC_EVENT_HANDLER_ERR;
            }

            pChnVars->stStream.u32PackCount = stStat.u32CurPacks;
            s32Ret = pEncCtx->base.getStream(pEncCtx, &pChnVars->stStream,
                            s32MilliSec);
            pChnVars->s32BindModeGetStreamRet = s32Ret;
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("getStream, VencChn = %d, s32Ret = %d\n",
                        VencChn, s32Ret);
                SEMA_POST(&pChnVars->sem_send);
                goto VENC_EVENT_HANDLER_ERR;
            }

            vb_done_handler(chn, CHN_TYPE_IN, &pVbCtx->vb_jobs, blk);
            s32Ret = cviProcessResult(pChnHandle, &pChnVars->stStream);
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("(chn %d) cviProcessResult fail\n",
                        VencChn);
                SEMA_POST(&pChnVars->sem_send);
                goto VENC_EVENT_HANDLER_ERR_2;
            }

            SEMA_POST(&pChnVars->sem_send);

            CVI_VENC_INFO("[%d]wait release\n", vi_cnt);
            if (SEMA_WAIT(&pChnVars->sem_release) != 0) {
                CVI_VENC_ERR("can not down sem_release\n");
                goto VENC_EVENT_HANDLER_ERR_2;
            }

            s32Ret = pEncCtx->base.releaseStream(pEncCtx,
                                &pChnVars->stStream);
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("[Vench %d]releaseStream ,s32Ret = %d\n",
                        VencChn, s32Ret);
                goto VENC_EVENT_HANDLER_ERR_2;
            }

            MEM_FREE(pChnVars->stStream.pstPack);
            pChnVars->stStream.pstPack = NULL;
        } else {
            cviWaitEncodeDone(pChnHandle);
            vb_done_handler(chn, CHN_TYPE_IN, &pVbCtx->vb_jobs, blk);
        }

        vi_cnt++;
        cond_resched();
    }

    CVI_VENC_INFO("------------end\n");
    #ifdef DUMP_BIND_YUV
    fclose(out_f);
    #endif
    return CVI_SUCCESS;

VENC_EVENT_HANDLER_ERR:
    vb_done_handler(chn, CHN_TYPE_IN, &pVbCtx->vb_jobs, blk);

VENC_EVENT_HANDLER_ERR_2:
    CVI_VENC_INFO("end\n");

#ifdef DUMP_BIND_YUV
    fclose(out_f);
#endif

    return CVI_SUCCESS;
}

CVI_S32 check_chn_handle(VENC_CHN VeChn)
{
    if (handle == NULL) {
        CVI_VENC_ERR("Call VENC Destroy before Create, failed\n");
        return CVI_ERR_VENC_UNEXIST;
    }

    if (handle->chn_handle[VeChn] == NULL) {
        CVI_VENC_ERR("VENC Chn #%d haven't created !\n", VeChn);
        return CVI_ERR_VENC_INVALID_CHNID;
    }

    return CVI_SUCCESS;
}


static void _cviVencInitModParam(CVI_VENC_PARAM_MOD_S *pModParam)
{
    if (!pModParam)
        return;

    memset(pModParam, 0, sizeof(*pModParam));
    pModParam->stJpegeModParam.JpegMarkerOrder[0] = JPEGE_MARKER_SOI;
    pModParam->stJpegeModParam.JpegMarkerOrder[1] =
        JPEGE_MARKER_FRAME_INDEX;
    pModParam->stJpegeModParam.JpegMarkerOrder[2] = JPEGE_MARKER_USER_DATA;
    pModParam->stJpegeModParam.JpegMarkerOrder[3] = JPEGE_MARKER_DRI_OPT;
    pModParam->stJpegeModParam.JpegMarkerOrder[4] = JPEGE_MARKER_DQT;
    pModParam->stJpegeModParam.JpegMarkerOrder[5] = JPEGE_MARKER_DHT;
    pModParam->stJpegeModParam.JpegMarkerOrder[6] = JPEGE_MARKER_SOF0;
    pModParam->stJpegeModParam.JpegMarkerOrder[7] = JPEGE_MARKER_BUTT;
    pModParam->stH264eModParam.u32UserDataMaxLen = USERDATA_MAX_DEFAULT;
    pModParam->stH265eModParam.u32UserDataMaxLen = USERDATA_MAX_DEFAULT;
}

static CVI_S32 _cviVencInitCtxHandle(void)
{
    venc_context *pVencHandle;

    if (handle == NULL) {
        handle = MEM_CALLOC(1, sizeof(venc_context));
        if (handle == NULL) {
            CVI_VENC_ERR("venc_context create failure\n");
            return CVI_ERR_VENC_NOMEM;
        }

        pVencHandle = (venc_context *)handle;
        _cviVencInitModParam(&pVencHandle->ModParam);
    }

    return CVI_SUCCESS;
}


int cviVencDrvInit(void)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    base_register_recv_cb(CVI_ID_VENC, venc_recv_qbuf);

    s32Ret = _cviVencInitCtxHandle();
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("venc_context\n");
        s32Ret = CVI_ERR_VENC_NOMEM;
    }
    return s32Ret;
}

void cviVencDrvDeinit(void)
{
    base_unregister_recv_cb(CVI_ID_VENC);
    if(handle)
        MEM_FREE(handle);
    handle = NULL;
    return;
}

static CVI_S32 cviInitFrcSoftFloat(venc_chn_context *pChnHandle,
        CVI_U32 u32SrcFrameRate, CVI_FR32 fr32DstFrameRate)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;
    venc_frc *pvf = &pChnVars->frc;
    CVI_S32 s32Ret = CVI_SUCCESS;
    CVI_S32 dstFrDenom;
    CVI_S32 dstFrfract;
    float32 srcFrameRate;
    float32 dstFrameRate;
    int srcFrChecker, dstFrChecker;

    dstFrDenom = (fr32DstFrameRate >> 16) & 0xFFFF;
    dstFrfract = (fr32DstFrameRate & 0xFFFF);

    dstFrameRate = INT_TO_CVI_FLOAT(dstFrfract);
    if (dstFrDenom != 0)
        dstFrameRate = CVI_FLOAT_DIV(dstFrameRate, INT_TO_CVI_FLOAT(dstFrDenom));

    dstFrDenom = (u32SrcFrameRate >> 16) & 0xFFFF;
    dstFrfract = (u32SrcFrameRate & 0xFFFF);

    srcFrameRate = INT_TO_CVI_FLOAT(dstFrfract);
    if (dstFrDenom != 0)
        srcFrameRate = CVI_FLOAT_DIV(srcFrameRate, INT_TO_CVI_FLOAT(dstFrDenom));

    srcFrChecker = CVI_FLOAT_TO_INT(CVI_FLOAT_MUL(srcFrameRate, FLOAT_VAL_10000));
    dstFrChecker = CVI_FLOAT_TO_INT(CVI_FLOAT_MUL(dstFrameRate, FLOAT_VAL_10000));

    if (srcFrChecker > dstFrChecker) {
        if (CVI_FLOAT_EQ(srcFrameRate, FLOAT_VAL_0) ||
            CVI_FLOAT_EQ(dstFrameRate, FLOAT_VAL_0)) {
            CVI_VENC_ERR(
                "Dst frame rate(%d), Src frame rate(%d), not supported\n",
                dstFrameRate, srcFrameRate);
            pvf->bFrcEnable = CVI_FALSE;
            s32Ret = CVI_ERR_VENC_NOT_SUPPORT;
            return s32Ret;
        }
        pvf->bFrcEnable = CVI_TRUE;
        pvf->srcFrameDur =
            CVI_FLOAT_TO_INT(CVI_FLOAT_DIV(FLOAT_VAL_FRC_TIME_SCALE, srcFrameRate));
        pvf->dstFrameDur =
            CVI_FLOAT_TO_INT(CVI_FLOAT_DIV(FLOAT_VAL_FRC_TIME_SCALE, dstFrameRate));
        pvf->srcTs = pvf->srcFrameDur;
        pvf->dstTs = pvf->dstFrameDur;
    } else if (srcFrChecker == dstFrChecker) {
        pvf->bFrcEnable = CVI_FALSE;
    } else {
        pvf->bFrcEnable = CVI_FALSE;
        CVI_VENC_ERR(
            "Dst frame rate(%d) > Src frame rate(%d), not supported\n",
            dstFrameRate, srcFrameRate);
        s32Ret = CVI_ERR_VENC_NOT_SUPPORT;
        return s32Ret;
    }

    return s32Ret;
}


static CVI_S32 cviInitFrc(venc_chn_context *pChnHandle,
        CVI_U32 u32SrcFrameRate, CVI_FR32 fr32DstFrameRate)
{
    CVI_S32 s32Ret = CVI_SUCCESS;

    s32Ret = cviInitFrcSoftFloat(pChnHandle, u32SrcFrameRate, fr32DstFrameRate);

    return s32Ret;
}


static CVI_S32 _cviCheckFrameRate(venc_chn_context *pChnHandle,
                  CVI_U32 *pu32SrcFrameRate,
                  CVI_FR32 *pfr32DstFrameRate,
                  CVI_BOOL bVariFpsEn)
{
    CVI_S32 s32Ret = CVI_SUCCESS;

    int frameRateDiv, frameRateRes;
    // TODO: use soft-floating to replace it
    int fSrcFrmrate, fDstFrmrate;

    s32Ret = (bVariFpsEn <= CVI_VARI_FPS_EN_MAX) ?
        CVI_SUCCESS : CVI_ERR_VENC_ILLEGAL_PARAM;
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("error, %s = %d\n",
            "bVariFpsEn", bVariFpsEn);
        return s32Ret;
    }

    if (*pfr32DstFrameRate == 0) {
        CVI_VENC_WARN("set fr32DstFrameRate to %d\n",
                  DEST_FRAMERATE_DEF);
        *pfr32DstFrameRate = DEST_FRAMERATE_DEF;
    }

    if (*pu32SrcFrameRate == 0) {
        CVI_VENC_WARN("set u32SrcFrameRate to %d\n",
                  *pfr32DstFrameRate);
        *pu32SrcFrameRate = *pfr32DstFrameRate;
    }

    frameRateDiv = (*pfr32DstFrameRate >> 16);
    frameRateRes = *pfr32DstFrameRate & 0xFFFF;

    if (frameRateDiv == 0)
        fDstFrmrate = frameRateRes;
    else
        fDstFrmrate = frameRateRes / frameRateDiv;

    frameRateDiv = (*pu32SrcFrameRate >> 16);
    frameRateRes = *pu32SrcFrameRate & 0xFFFF;

    if (frameRateDiv == 0)
        fSrcFrmrate = frameRateRes;
    else
        fSrcFrmrate = frameRateRes / frameRateDiv;

    if (fDstFrmrate > DEST_FRAMERATE_MAX ||
        fDstFrmrate < DEST_FRAMERATE_MIN) {
        CVI_VENC_ERR("fr32DstFrameRate = 0x%X, not support\n",
                 *pfr32DstFrameRate);
        return CVI_ERR_VENC_NOT_SUPPORT;
    }

    if (fSrcFrmrate > SRC_FRAMERATE_MAX ||
        fSrcFrmrate < SRC_FRAMERATE_MIN) {
        CVI_VENC_ERR("u32SrcFrameRate = %d, not support\n",
                 *pu32SrcFrameRate);
        return CVI_ERR_VENC_NOT_SUPPORT;
    }

    if (fDstFrmrate > fSrcFrmrate) {
        *pfr32DstFrameRate = *pu32SrcFrameRate;
        CVI_VENC_WARN("fDstFrmrate > fSrcFrmrate\n");
        CVI_VENC_WARN("=> fr32DstFrameRate = u32SrcFrameRate\n");
    }

    s32Ret = cviInitFrc(pChnHandle, *pu32SrcFrameRate, *pfr32DstFrameRate);

    return s32Ret;
}

static CVI_S32 _cviVencRegVbBuf(VENC_CHN VeChn)
{
    CVI_S32 s32Ret = CVI_SUCCESS;

    venc_context *pVencHandle = handle;
    venc_enc_ctx *pEncCtx = NULL;

    venc_chn_context *pChnHandle = NULL;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    if (pVencHandle == NULL) {
        CVI_VENC_ERR(
            "p_venctx_handle NULL (Channel not create yet..)\n");
        return CVI_ERR_VENC_NULL_PTR;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;

    if (pEncCtx->base.ioctl) {
        s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_REG_VB_BUFFER,
                         NULL);
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("CVI_H26X_OP_REG_VB_BUFFER, %d\n", s32Ret);
            return s32Ret;
        }
    }

    return s32Ret;
}

static CVI_S32 _cviVencSetInPixelFormat(VENC_CHN VeChn,
                    CVI_BOOL bCbCrInterleave,
                    CVI_BOOL bNV21)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_enc_ctx *pEncCtx = NULL;
    venc_chn_context *pChnHandle = NULL;
    cviInPixelFormat inPixelFormat;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;

    inPixelFormat.bCbCrInterleave = bCbCrInterleave;
    inPixelFormat.bNV21 = bNV21;

    s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_IN_PIXEL_FORMAT,
                     (CVI_VOID *)&inPixelFormat);

    return s32Ret;
}

static CVI_S32 cviCheckRcModeAttr(venc_chn_context *pChnHandle)
{
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    CVI_S32 s32Ret = CVI_SUCCESS;

    if (pChnAttr->stVencAttr.enType == PT_MJPEG) {
        if (pChnAttr->stRcAttr.enRcMode == VENC_RC_MODE_MJPEGCBR) {
            VENC_MJPEG_CBR_S *pstMjpegeCbr =
                &pChnAttr->stRcAttr.stMjpegCbr;

            s32Ret = _cviCheckFrameRate(
                pChnHandle, &pstMjpegeCbr->u32SrcFrameRate,
                &pstMjpegeCbr->fr32DstFrameRate,
                pstMjpegeCbr->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
               VENC_RC_MODE_MJPEGFIXQP) {
            VENC_MJPEG_FIXQP_S *pstMjpegeFixQp =
                &pChnAttr->stRcAttr.stMjpegFixQp;

            s32Ret = _cviCheckFrameRate(
                pChnHandle, &pstMjpegeFixQp->u32SrcFrameRate,
                &pstMjpegeFixQp->fr32DstFrameRate,
                pstMjpegeFixQp->bVariFpsEn);
        } else {
            s32Ret = CVI_ERR_VENC_NOT_SUPPORT;
            CVI_VENC_ERR("enRcMode = %d, not support\n",
                     pChnAttr->stRcAttr.enRcMode);
        }
    } else if (pChnAttr->stVencAttr.enType == PT_H264) {
        if (pChnAttr->stRcAttr.enRcMode == VENC_RC_MODE_H264CBR) {
            VENC_H264_CBR_S *pstH264Cbr =
                &pChnAttr->stRcAttr.stH264Cbr;

            s32Ret = _cviCheckFrameRate(
                pChnHandle, &pstH264Cbr->u32SrcFrameRate,
                &pstH264Cbr->fr32DstFrameRate,
                pstH264Cbr->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
               VENC_RC_MODE_H264VBR) {
            VENC_H264_VBR_S *pstH264Vbr =
                &pChnAttr->stRcAttr.stH264Vbr;

            s32Ret = _cviCheckFrameRate(
                pChnHandle, &pstH264Vbr->u32SrcFrameRate,
                &pstH264Vbr->fr32DstFrameRate,
                pstH264Vbr->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
               VENC_RC_MODE_H264AVBR) {
            VENC_H264_AVBR_S *pstH264AVbr =
                &pChnAttr->stRcAttr.stH264AVbr;

            s32Ret = _cviCheckFrameRate(
                pChnHandle, &pstH264AVbr->u32SrcFrameRate,
                &pstH264AVbr->fr32DstFrameRate,
                pstH264AVbr->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
               VENC_RC_MODE_H264FIXQP) {
            VENC_H264_FIXQP_S *pstH264FixQp =
                &pChnAttr->stRcAttr.stH264FixQp;

            s32Ret = _cviCheckFrameRate(
                pChnHandle, &pstH264FixQp->u32SrcFrameRate,
                &pstH264FixQp->fr32DstFrameRate,
                pstH264FixQp->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
               VENC_RC_MODE_H264UBR) {
            VENC_H264_UBR_S *pstH264Ubr =
                &pChnAttr->stRcAttr.stH264Ubr;

            s32Ret = _cviCheckFrameRate(
                pChnHandle, &pstH264Ubr->u32SrcFrameRate,
                &pstH264Ubr->fr32DstFrameRate,
                pstH264Ubr->bVariFpsEn);
        } else {
            s32Ret = CVI_ERR_VENC_NOT_SUPPORT;
            CVI_VENC_ERR("enRcMode = %d, not support\n",
                     pChnAttr->stRcAttr.enRcMode);
        }
    } else if (pChnAttr->stVencAttr.enType == PT_H265) {
        if (pChnAttr->stRcAttr.enRcMode == VENC_RC_MODE_H265CBR) {
            VENC_H265_CBR_S *pstH265Cbr =
                &pChnAttr->stRcAttr.stH265Cbr;

            s32Ret = _cviCheckFrameRate(
                pChnHandle, &pstH265Cbr->u32SrcFrameRate,
                &pstH265Cbr->fr32DstFrameRate,
                pstH265Cbr->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
               VENC_RC_MODE_H265VBR) {
            VENC_H265_VBR_S *pstH265Vbr =
                &pChnAttr->stRcAttr.stH265Vbr;

            s32Ret = _cviCheckFrameRate(
                pChnHandle, &pstH265Vbr->u32SrcFrameRate,
                &pstH265Vbr->fr32DstFrameRate,
                pstH265Vbr->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
               VENC_RC_MODE_H265AVBR) {
            VENC_H265_AVBR_S *pstH265AVbr =
                &pChnAttr->stRcAttr.stH265AVbr;

            s32Ret = _cviCheckFrameRate(
                pChnHandle, &pstH265AVbr->u32SrcFrameRate,
                &pstH265AVbr->fr32DstFrameRate,
                pstH265AVbr->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
               VENC_RC_MODE_H265FIXQP) {
            VENC_H265_FIXQP_S *pstH265FixQp =
                &pChnAttr->stRcAttr.stH265FixQp;

            s32Ret = _cviCheckFrameRate(
                pChnHandle, &pstH265FixQp->u32SrcFrameRate,
                &pstH265FixQp->fr32DstFrameRate,
                pstH265FixQp->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
               VENC_RC_MODE_H265QPMAP) {
            VENC_H265_QPMAP_S *pstH265QpMap =
                &pChnAttr->stRcAttr.stH265QpMap;

            s32Ret = _cviCheckFrameRate(
                pChnHandle, &pstH265QpMap->u32SrcFrameRate,
                &pstH265QpMap->fr32DstFrameRate,
                pstH265QpMap->bVariFpsEn);
        } else if (pChnAttr->stRcAttr.enRcMode ==
               VENC_RC_MODE_H265UBR) {
            VENC_H265_UBR_S *pstH265Ubr =
                &pChnAttr->stRcAttr.stH265Ubr;

            s32Ret = _cviCheckFrameRate(
                pChnHandle, &pstH265Ubr->u32SrcFrameRate,
                &pstH265Ubr->fr32DstFrameRate,
                pstH265Ubr->bVariFpsEn);
        } else {
            s32Ret = CVI_ERR_VENC_NOT_SUPPORT;
            CVI_VENC_ERR("enRcMode = %d, not support\n",
                     pChnAttr->stRcAttr.enRcMode);
        }
    }

    return s32Ret;
}

static CVI_U32 _cviGetGop(const VENC_RC_ATTR_S *pRcAttr)
{
    switch (pRcAttr->enRcMode) {
    case VENC_RC_MODE_H264CBR:
        return pRcAttr->stH264Cbr.u32Gop;
    case VENC_RC_MODE_H264VBR:
        return pRcAttr->stH264Vbr.u32Gop;
    case VENC_RC_MODE_H264AVBR:
        return pRcAttr->stH264AVbr.u32Gop;
    case VENC_RC_MODE_H264QVBR:
        return pRcAttr->stH264QVbr.u32Gop;
    case VENC_RC_MODE_H264FIXQP:
        return pRcAttr->stH264FixQp.u32Gop;
    case VENC_RC_MODE_H264QPMAP:
        return pRcAttr->stH264QpMap.u32Gop;
    case VENC_RC_MODE_H264UBR:
        return pRcAttr->stH264Ubr.u32Gop;
    case VENC_RC_MODE_H265CBR:
        return pRcAttr->stH265Cbr.u32Gop;
    case VENC_RC_MODE_H265VBR:
        return pRcAttr->stH265Vbr.u32Gop;
    case VENC_RC_MODE_H265AVBR:
        return pRcAttr->stH265AVbr.u32Gop;
    case VENC_RC_MODE_H265QVBR:
        return pRcAttr->stH265QVbr.u32Gop;
    case VENC_RC_MODE_H265FIXQP:
        return pRcAttr->stH265FixQp.u32Gop;
    case VENC_RC_MODE_H265QPMAP:
        return pRcAttr->stH265QpMap.u32Gop;
    case VENC_RC_MODE_H265UBR:
        return pRcAttr->stH265Ubr.u32Gop;
    default:
        return 0;
    }
}

static CVI_S32 cviCheckGopAttr(venc_chn_context *pChnHandle)
{
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    VENC_RC_ATTR_S *pRcAttr = &pChnAttr->stRcAttr;
    VENC_GOP_ATTR_S *pGopAttr = &pChnAttr->stGopAttr;
    CVI_U32 u32Gop = _cviGetGop(pRcAttr);
    CVI_S32 s32Ret;

    if (pChnAttr->stVencAttr.enType == PT_JPEG ||
        pChnAttr->stVencAttr.enType == PT_MJPEG) {
        return CVI_SUCCESS;
    }

    if (u32Gop == 0) {
        CVI_VENC_ERR("enRcMode = %d, not support\n", pRcAttr->enRcMode);
        return CVI_ERR_VENC_RC_PARAM;
    }

    switch (pGopAttr->enGopMode) {
    case VENC_GOPMODE_NORMALP:
        s32Ret = ((pGopAttr->stNormalP.s32IPQpDelta >= CVI_H26X_NORMALP_IP_QP_DELTA_MIN) &&
            (pGopAttr->stNormalP.s32IPQpDelta <= CVI_H26X_NORMALP_IP_QP_DELTA_MAX)) ?
            CVI_SUCCESS : CVI_ERR_VENC_GOP_ATTR;
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("error, %s = %d\n",
                "s32IPQpDelta", pGopAttr->stNormalP.s32IPQpDelta);
            return s32Ret;
        }
        break;

    case VENC_GOPMODE_SMARTP:
        s32Ret = ((pGopAttr->stSmartP.u32BgInterval >= u32Gop) &&
            (pGopAttr->stSmartP.u32BgInterval <= CVI_H26X_SMARTP_BG_INTERVAL_MAX)) ?
            CVI_SUCCESS : CVI_ERR_VENC_GOP_ATTR;
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("error, %s = %d\n",
                "u32BgInterval", pGopAttr->stNormalP.s32IPQpDelta);
            return s32Ret;
        }
        s32Ret = ((pGopAttr->stSmartP.s32BgQpDelta >= CVI_H26X_SMARTP_BG_QP_DELTA_MIN) &&
            (pGopAttr->stSmartP.s32BgQpDelta <= CVI_H26X_SMARTP_BG_QP_DELTA_MAX)) ?
            CVI_SUCCESS : CVI_ERR_VENC_GOP_ATTR;
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("error, %s = %d\n",
                "s32BgQpDelta", pGopAttr->stNormalP.s32IPQpDelta);
            return s32Ret;
        }
        s32Ret = ((pGopAttr->stSmartP.s32ViQpDelta >= CVI_H26X_SMARTP_VI_QP_DELTA_MIN) &&
            (pGopAttr->stSmartP.s32ViQpDelta <= CVI_H26X_SMARTP_VI_QP_DELTA_MAX)) ?
            CVI_SUCCESS : CVI_ERR_VENC_GOP_ATTR;
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("error, %s = %d\n",
                "s32ViQpDelta", pGopAttr->stSmartP.s32ViQpDelta);
            return s32Ret;
        }
        if ((pGopAttr->stSmartP.u32BgInterval % u32Gop) != 0) {
            CVI_VENC_ERR(
                "u32BgInterval %d, not a multiple of u32Gop %d\n",
                pGopAttr->stAdvSmartP.u32BgInterval, u32Gop);
            return CVI_ERR_VENC_GOP_ATTR;
        }
        break;

    default:
        CVI_VENC_ERR("enGopMode = %d, not support\n",
                 pGopAttr->enGopMode);
        return CVI_ERR_VENC_GOP_ATTR;
    }

    return CVI_SUCCESS;
}

static CVI_S32 cviCheckGopExAttr(venc_chn_context *pChnHandle)
{
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    VENC_GOP_EX_ATTR_S *pGopExAttr = &pChnAttr->stGopExAttr;

    if (pChnAttr->stVencAttr.enType == PT_JPEG ||
        pChnAttr->stVencAttr.enType == PT_MJPEG) {
        return CVI_SUCCESS;
    }

    if (pGopExAttr->u32GopPreset > GOP_PRESET_IDX_IPP_SINGLE) {
        CVI_VENC_ERR("invalid goppreset:%d\n", pGopExAttr->u32GopPreset);
        return CVI_ERR_VENC_GOP_ATTR;
    }

    if (pGopExAttr->u32GopPreset == GOP_PRESET_IDX_CUSTOM_GOP
        && (pGopExAttr->gopParam.s32CustomGopSize < 1 || pGopExAttr->gopParam.s32CustomGopSize > 8)) {
        CVI_VENC_ERR("invalid gopparam size:%d when goppreset is 0\n", pGopExAttr->u32GopPreset);
        return CVI_ERR_VENC_GOP_ATTR;
    }

    return CVI_SUCCESS;
}

static CVI_S32 cviSetDefaultRcParam(venc_chn_context *pChnHandle)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    VENC_CHN_ATTR_S *pChnAttr;
    VENC_ATTR_S *pVencAttr;
    VENC_RC_ATTR_S *prcatt;
    VENC_RC_PARAM_S *prcparam;

    pChnAttr = pChnHandle->pChnAttr;
    pVencAttr = &pChnAttr->stVencAttr;
    prcatt = &pChnAttr->stRcAttr;
    prcparam = &pChnHandle->rcParam;

    prcparam->u32RowQpDelta = CVI_H26X_ROW_QP_DELTA_DEFAULT;
    prcparam->s32FirstFrameStartQp =
        ((pVencAttr->enType == PT_H265) ? 63 : DEF_IQP);
    prcparam->s32InitialDelay = CVI_INITIAL_DELAY_DEFAULT;
    prcparam->u32ThrdLv = CVI_H26X_THRDLV_DEFAULT;
    prcparam->bBgEnhanceEn = CVI_H26X_BG_ENHANCE_EN_DEFAULT;
    prcparam->s32BgDeltaQp = CVI_H26X_BG_DELTA_QP_DEFAULT;

    if (pVencAttr->enType == PT_H264) {
        if (prcatt->enRcMode == VENC_RC_MODE_H264CBR) {
            VENC_PARAM_H264_CBR_S *pstParamH264Cbr =
                &prcparam->stParamH264Cbr;

            SET_DEFAULT_RC_PARAM(pstParamH264Cbr);
        } else if (prcatt->enRcMode == VENC_RC_MODE_H264VBR) {
            VENC_PARAM_H264_VBR_S *pstParamH264Vbr =
                &prcparam->stParamH264Vbr;

            SET_DEFAULT_RC_PARAM(pstParamH264Vbr);
            pstParamH264Vbr->s32ChangePos =
                CVI_H26X_CHANGE_POS_DEFAULT;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H264AVBR) {
            VENC_PARAM_H264_AVBR_S *pstParamH264AVbr =
                &prcparam->stParamH264AVbr;

            SET_DEFAULT_RC_PARAM(pstParamH264AVbr);
            pstParamH264AVbr->s32ChangePos =
                CVI_H26X_CHANGE_POS_DEFAULT;
            pstParamH264AVbr->s32MinStillPercent =
                CVI_H26X_MIN_STILL_PERCENT_DEFAULT;
            pstParamH264AVbr->u32MaxStillQP =
                CVI_H26X_MAX_STILL_QP_DEFAULT;
            pstParamH264AVbr->u32MotionSensitivity =
                CVI_H26X_MOTION_SENSITIVITY_DEFAULT;
            pstParamH264AVbr->s32AvbrFrmLostOpen =
                CVI_H26X_AVBR_FRM_LOST_OPEN_DEFAULT;
            pstParamH264AVbr->s32AvbrFrmGap =
                CVI_H26X_AVBR_FRM_GAP_DEFAULT;
            pstParamH264AVbr->s32AvbrPureStillThr =
                CVI_H26X_AVBR_PURE_STILL_THR_DEFAULT;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H264UBR) {
            VENC_PARAM_H264_UBR_S *pstParamH264Ubr =
                &prcparam->stParamH264Ubr;

            SET_DEFAULT_RC_PARAM(pstParamH264Ubr);
        }
    } else if (pVencAttr->enType == PT_H265) {
        if (prcatt->enRcMode == VENC_RC_MODE_H265CBR) {
            VENC_PARAM_H265_CBR_S *pstParamH265Cbr =
                &prcparam->stParamH265Cbr;

            SET_DEFAULT_RC_PARAM(pstParamH265Cbr);
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265VBR) {
            VENC_PARAM_H265_VBR_S *pstParamH265Vbr =
                &prcparam->stParamH265Vbr;

            SET_DEFAULT_RC_PARAM(pstParamH265Vbr);
            pstParamH265Vbr->s32ChangePos =
                CVI_H26X_CHANGE_POS_DEFAULT;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265AVBR) {
            VENC_PARAM_H265_AVBR_S *pstParamH265AVbr =
                &prcparam->stParamH265AVbr;

            SET_DEFAULT_RC_PARAM(pstParamH265AVbr);
            pstParamH265AVbr->s32ChangePos =
                CVI_H26X_CHANGE_POS_DEFAULT;
            pstParamH265AVbr->s32MinStillPercent =
                CVI_H26X_MIN_STILL_PERCENT_DEFAULT;
            pstParamH265AVbr->u32MaxStillQP =
                CVI_H26X_MAX_STILL_QP_DEFAULT;
            pstParamH265AVbr->u32MotionSensitivity =
                CVI_H26X_MOTION_SENSITIVITY_DEFAULT;
            pstParamH265AVbr->s32AvbrFrmLostOpen =
                CVI_H26X_AVBR_FRM_LOST_OPEN_DEFAULT;
            pstParamH265AVbr->s32AvbrFrmGap =
                CVI_H26X_AVBR_FRM_GAP_DEFAULT;
            pstParamH265AVbr->s32AvbrPureStillThr =
                CVI_H26X_AVBR_PURE_STILL_THR_DEFAULT;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265QPMAP) {
            VENC_PARAM_H265_CBR_S *pstParamH265Cbr =
                &prcparam->stParamH265Cbr;

            // When using QP Map, we use CBR as basic setting
            SET_DEFAULT_RC_PARAM(pstParamH265Cbr);
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265UBR) {
            VENC_PARAM_H265_UBR_S *pstParamH265Ubr =
                &prcparam->stParamH265Ubr;

            SET_DEFAULT_RC_PARAM(pstParamH265Ubr);
        }
    }

    return s32Ret;
}

static CVI_VOID cviInitVfps(venc_chn_context *pChnHandle)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;
    venc_vfps *pVfps = &pChnVars->vfps;

    memset(pVfps, 0, sizeof(venc_vfps));
    pVfps->u64StatTime = CVI_DEF_VFPFS_STAT_TIME * 1000 * 1000;
}


static CVI_S32 cviSetChnDefault(venc_chn_context *pChnHandle)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    VENC_ATTR_S *pVencAttr = &pChnAttr->stVencAttr;
    VENC_JPEG_PARAM_S *pvjp = &pChnVars->stJpegParam;
    VENC_CU_PREDICTION_S *pcup = &pChnVars->cuPrediction;
    VENC_SUPERFRAME_CFG_S *pcsf = &pChnVars->stSuperFrmParam;
    VENC_FRAME_PARAM_S *pfp = &pChnVars->frameParam;
    CVI_S32 s32Ret = CVI_SUCCESS;
    MMF_CHN_S chn = { .enModId = CVI_ID_VENC,
              .s32DevId = 0,
              .s32ChnId = pChnHandle->VeChn };

    if (pVencAttr->enType == PT_H264) {
        pChnHandle->h264Vui.stVuiAspectRatio.aspect_ratio_idc =
            CVI_H26X_ASPECT_RATIO_IDC_DEFAULT;
        pChnHandle->h264Vui.stVuiAspectRatio.sar_width =
            CVI_H26X_SAR_WIDTH_DEFAULT;
        pChnHandle->h264Vui.stVuiAspectRatio.sar_height =
            CVI_H26X_SAR_HEIGHT_DEFAULT;
        pChnHandle->h264Vui.stVuiTimeInfo.num_units_in_tick =
            CVI_H26X_NUM_UNITS_IN_TICK_DEFAULT;
        pChnHandle->h264Vui.stVuiTimeInfo.time_scale =
            CVI_H26X_TIME_SCALE_DEFAULT;
        pChnHandle->h264Vui.stVuiVideoSignal.video_format =
            CVI_H26X_VIDEO_FORMAT_DEFAULT;
        pChnHandle->h264Vui.stVuiVideoSignal.colour_primaries =
            CVI_H26X_COLOUR_PRIMARIES_DEFAULT;
        pChnHandle->h264Vui.stVuiVideoSignal.transfer_characteristics =
            CVI_H26X_TRANSFER_CHARACTERISTICS_DEFAULT;
        pChnHandle->h264Vui.stVuiVideoSignal.matrix_coefficients =
            CVI_H26X_MATRIX_COEFFICIENTS_DEFAULT;
    } else if (pVencAttr->enType == PT_H265) {
        pChnHandle->h265Vui.stVuiAspectRatio.aspect_ratio_idc =
            CVI_H26X_ASPECT_RATIO_IDC_DEFAULT;
        pChnHandle->h265Vui.stVuiAspectRatio.sar_width =
            CVI_H26X_SAR_WIDTH_DEFAULT;
        pChnHandle->h265Vui.stVuiAspectRatio.sar_height =
            CVI_H26X_SAR_HEIGHT_DEFAULT;
        pChnHandle->h265Vui.stVuiTimeInfo.num_units_in_tick =
            CVI_H26X_NUM_UNITS_IN_TICK_DEFAULT;
        pChnHandle->h265Vui.stVuiTimeInfo.time_scale =
            CVI_H26X_TIME_SCALE_DEFAULT;
        pChnHandle->h265Vui.stVuiTimeInfo.num_ticks_poc_diff_one_minus1 =
            CVI_H265_NUM_TICKS_POC_DIFF_ONE_MINUS1_DEFAULT;
        pChnHandle->h265Vui.stVuiVideoSignal.video_format =
            CVI_H26X_VIDEO_FORMAT_DEFAULT;
        pChnHandle->h265Vui.stVuiVideoSignal.colour_primaries =
            CVI_H26X_COLOUR_PRIMARIES_DEFAULT;
        pChnHandle->h265Vui.stVuiVideoSignal.transfer_characteristics =
            CVI_H26X_TRANSFER_CHARACTERISTICS_DEFAULT;
        pChnHandle->h265Vui.stVuiVideoSignal.matrix_coefficients =
            CVI_H26X_MATRIX_COEFFICIENTS_DEFAULT;
    }

    if (pVencAttr->enType == PT_H264) {
        if (pVencAttr->u32Profile == H264E_PROFILE_BASELINE) {
            pChnHandle->h264Entropy.u32EntropyEncModeI =
                H264E_ENTROPY_CAVLC;
            pChnHandle->h264Entropy.u32EntropyEncModeP =
                H264E_ENTROPY_CAVLC;
        } else {
            pChnHandle->h264Entropy.u32EntropyEncModeI =
                H264E_ENTROPY_CABAC;
            pChnHandle->h264Entropy.u32EntropyEncModeP =
                H264E_ENTROPY_CABAC;
        }
    }

    pvjp->u32Qfactor = Q_TABLE_DEFAULT;
    pcup->u32IntraCost = CVI_H26X_INTRACOST_DEFAULT;
    pcsf->enSuperFrmMode = CVI_H26X_SUPER_FRM_MODE_DEFAULT;
    pcsf->u32SuperIFrmBitsThr = CVI_H26X_SUPER_I_BITS_THR_DEFAULT;
    pcsf->u32SuperPFrmBitsThr = CVI_H26X_SUPER_P_BITS_THR_DEFAULT;
    pfp->u32FrameQp = CVI_H26X_FRAME_QP_DEFAULT;
    pfp->u32FrameBits = CVI_H26X_FRAME_BITS_DEFAULT;

    s32Ret = cviSetDefaultRcParam(pChnHandle);
    if (s32Ret < 0) {
        CVI_VENC_ERR("cviSetDefaultRcParam\n");
        return s32Ret;
    }

    pChnVars->vbpool.hPicInfoVbPool = VB_INVALID_POOLID;
    chn.s32ChnId = pChnHandle->VeChn;
    base_mod_jobs_init(&vencVbCtx[pChnHandle->VeChn].vb_jobs, 1, 1, 0);
    SEMA_INIT(&pChnVars->sem_send, 0, 0);
    SEMA_INIT(&pChnVars->sem_release, 0, 0);

    cviInitVfps(pChnHandle);

    return s32Ret;
}

static CVI_S32 cviSetRcParamToDrv(venc_chn_context *pChnHandle)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;
    VENC_RC_PARAM_S *prcparam = &pChnHandle->rcParam;
    VENC_ATTR_S *pVencAttr = &pChnAttr->stVencAttr;
    VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;

    if (pEncCtx->base.ioctl) {
        cviRcParam rcp, *prcp = &rcp;

        prcp->u32RowQpDelta = prcparam->u32RowQpDelta;
        prcp->firstFrmstartQp = prcparam->s32FirstFrameStartQp;
        prcp->u32ThrdLv = prcparam->u32ThrdLv;
        prcp->s32InitialDelay = prcparam->s32InitialDelay;
        prcp->s32ChangePos = 0;

        if (pVencAttr->enType == PT_H264) {
            if (prcatt->enRcMode == VENC_RC_MODE_H264CBR) {
                VENC_PARAM_H264_CBR_S *pstParamH264Cbr =
                    &prcparam->stParamH264Cbr;

                SET_COMMON_RC_PARAM(prcp, pstParamH264Cbr);
            } else if (prcatt->enRcMode == VENC_RC_MODE_H264VBR) {
                VENC_PARAM_H264_VBR_S *pstParamH264Vbr =
                    &prcparam->stParamH264Vbr;

                SET_COMMON_RC_PARAM(prcp, pstParamH264Vbr);
                prcp->s32ChangePos =
                    pstParamH264Vbr->s32ChangePos;
            } else if (prcatt->enRcMode == VENC_RC_MODE_H264AVBR) {
                VENC_PARAM_H264_AVBR_S *pstParamH264AVbr =
                    &prcparam->stParamH264AVbr;

                SET_COMMON_RC_PARAM(prcp, pstParamH264AVbr);
                prcp->s32ChangePos =
                    pstParamH264AVbr->s32ChangePos;
                prcp->s32MinStillPercent =
                    pstParamH264AVbr->s32MinStillPercent;
                prcp->u32MaxStillQP =
                    pstParamH264AVbr->u32MaxStillQP;
                prcp->u32MotionSensitivity =
                    pstParamH264AVbr->u32MotionSensitivity;
                prcp->s32AvbrFrmLostOpen =
                    pstParamH264AVbr->s32AvbrFrmLostOpen;
                prcp->s32AvbrFrmGap =
                    pstParamH264AVbr->s32AvbrFrmGap;
                prcp->s32AvbrPureStillThr =
                    pstParamH264AVbr->s32AvbrPureStillThr;

            } else if (prcatt->enRcMode == VENC_RC_MODE_H264UBR) {
                VENC_PARAM_H264_UBR_S *pstParamH264Ubr =
                    &prcparam->stParamH264Ubr;

                SET_COMMON_RC_PARAM(prcp, pstParamH264Ubr);
            }
        } else if (pVencAttr->enType == PT_H265) {
            if (prcatt->enRcMode == VENC_RC_MODE_H265CBR) {
                VENC_PARAM_H265_CBR_S *pstParamH265Cbr =
                    &prcparam->stParamH265Cbr;

                SET_COMMON_RC_PARAM(prcp, pstParamH265Cbr);
            } else if (prcatt->enRcMode == VENC_RC_MODE_H265VBR) {
                VENC_PARAM_H265_VBR_S *pstParamH265Vbr =
                    &prcparam->stParamH265Vbr;

                SET_COMMON_RC_PARAM(prcp, pstParamH265Vbr);
                prcp->s32ChangePos =
                    pstParamH265Vbr->s32ChangePos;
            } else if (prcatt->enRcMode == VENC_RC_MODE_H265AVBR) {
                VENC_PARAM_H265_AVBR_S *pstParamH265AVbr =
                    &prcparam->stParamH265AVbr;

                SET_COMMON_RC_PARAM(prcp, pstParamH265AVbr);
                prcp->s32ChangePos =
                    pstParamH265AVbr->s32ChangePos;
                prcp->s32MinStillPercent =
                    pstParamH265AVbr->s32MinStillPercent;
                prcp->u32MaxStillQP =
                    pstParamH265AVbr->u32MaxStillQP;
                prcp->u32MotionSensitivity =
                    pstParamH265AVbr->u32MotionSensitivity;
                prcp->s32AvbrFrmLostOpen =
                    pstParamH265AVbr->s32AvbrFrmLostOpen;
                prcp->s32AvbrFrmGap =
                    pstParamH265AVbr->s32AvbrFrmGap;
                prcp->s32AvbrPureStillThr =
                    pstParamH265AVbr->s32AvbrPureStillThr;
            } else if (prcatt->enRcMode == VENC_RC_MODE_H265QPMAP) {
                VENC_PARAM_H265_CBR_S *pstParamH265Cbr =
                    &prcparam->stParamH265Cbr;

                // When using QP Map, we use CBR as basic setting
                SET_COMMON_RC_PARAM(prcp, pstParamH265Cbr);
            } else if (prcatt->enRcMode == VENC_RC_MODE_H265UBR) {
                VENC_PARAM_H265_UBR_S *pstParamH265Ubr =
                    &prcparam->stParamH265Ubr;

                SET_COMMON_RC_PARAM(prcp, pstParamH265Ubr);
            }
        }

        s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_RC_PARAM,
                         (CVI_VOID *)prcp);
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("CVI_H26X_OP_SET_RC_PARAM, %d\n", s32Ret);
            return s32Ret;
        }
    } else {
        CVI_VENC_ERR("base.ioctl is NULL\n");
        return CVI_ERR_VENC_NULL_PTR;
    }

    return s32Ret;
}

CVI_S32 _cvi_h26x_trans_chn_attr(VENC_CHN_ATTR_S *pInChnAttr,
                 cviVidChnAttr *pOutAttr)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    VENC_RC_ATTR_S *pstRcAttr = NULL;

    if ((pInChnAttr == NULL) || (pOutAttr == NULL))
        return s32Ret;

    pstRcAttr = &pInChnAttr->stRcAttr;

    if (pInChnAttr->stVencAttr.enType == PT_H264) {
        if (pstRcAttr->enRcMode == VENC_RC_MODE_H264CBR) {
            pOutAttr->u32BitRate = pstRcAttr->stH264Cbr.u32BitRate;
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stH264Cbr.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stH264Cbr.fr32DstFrameRate;
            s32Ret = CVI_SUCCESS;
        } else if (pstRcAttr->enRcMode == VENC_RC_MODE_H264VBR) {
            pOutAttr->u32BitRate =
                pstRcAttr->stH264Vbr.u32MaxBitRate;
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stH264Vbr.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stH264Vbr.fr32DstFrameRate;
            s32Ret = CVI_SUCCESS;
        } else if (pstRcAttr->enRcMode == VENC_RC_MODE_H264AVBR) {
            pOutAttr->u32BitRate =
                pstRcAttr->stH264AVbr.u32MaxBitRate;
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stH264AVbr.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stH264AVbr.fr32DstFrameRate;
            s32Ret = CVI_SUCCESS;
        } else if (pstRcAttr->enRcMode == VENC_RC_MODE_H264UBR) {
            pOutAttr->u32BitRate = pstRcAttr->stH264Ubr.u32BitRate;
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stH264Ubr.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stH264Ubr.fr32DstFrameRate;
            s32Ret = CVI_SUCCESS;
        }
    } else if (pInChnAttr->stVencAttr.enType == PT_H265) {
        if (pstRcAttr->enRcMode == VENC_RC_MODE_H265CBR) {
            pOutAttr->u32BitRate = pstRcAttr->stH265Cbr.u32BitRate;
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stH265Cbr.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stH265Cbr.fr32DstFrameRate;
            s32Ret = CVI_SUCCESS;
        } else if (pstRcAttr->enRcMode == VENC_RC_MODE_H265VBR) {
            pOutAttr->u32BitRate =
                pstRcAttr->stH265Vbr.u32MaxBitRate;
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stH265Vbr.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stH265Vbr.fr32DstFrameRate;
            s32Ret = CVI_SUCCESS;
        } else if (pstRcAttr->enRcMode == VENC_RC_MODE_H265AVBR) {
            pOutAttr->u32BitRate =
                pstRcAttr->stH265AVbr.u32MaxBitRate;
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stH265AVbr.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stH265AVbr.fr32DstFrameRate;
            s32Ret = CVI_SUCCESS;
        } else if (pstRcAttr->enRcMode == VENC_RC_MODE_H265UBR) {
            pOutAttr->u32BitRate = pstRcAttr->stH265Ubr.u32BitRate;
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stH265Ubr.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stH265Ubr.fr32DstFrameRate;
            s32Ret = CVI_SUCCESS;
        }
    }

    return s32Ret;
}

CVI_S32 _cvi_jpg_trans_chn_attr(VENC_CHN_ATTR_S *pInChnAttr,
                cviJpegChnAttr *pOutAttr)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    VENC_RC_ATTR_S *pstRcAttr = NULL;
    VENC_ATTR_S *pstVencAttr = NULL;

    if ((pInChnAttr == NULL) || (pOutAttr == NULL))
        return s32Ret;

    pstRcAttr = &(pInChnAttr->stRcAttr);
    pstVencAttr = &(pInChnAttr->stVencAttr);

    if (pInChnAttr->stVencAttr.enType == PT_MJPEG) {
        pOutAttr->picWidth = pstVencAttr->u32PicWidth;
        pOutAttr->picHeight = pstVencAttr->u32PicHeight;
        if (pstRcAttr->enRcMode == VENC_RC_MODE_MJPEGCBR) {
            pOutAttr->u32BitRate = pstRcAttr->stMjpegCbr.u32BitRate;
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stMjpegCbr.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stMjpegCbr.fr32DstFrameRate;
            s32Ret = CVI_SUCCESS;
        } else if (pstRcAttr->enRcMode == VENC_RC_MODE_MJPEGVBR) {
            pOutAttr->u32BitRate =
                pstRcAttr->stMjpegVbr.u32MaxBitRate;
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stMjpegVbr.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stMjpegVbr.fr32DstFrameRate;
            s32Ret = CVI_SUCCESS;
        } else if (pstRcAttr->enRcMode == VENC_RC_MODE_MJPEGFIXQP) {
            pOutAttr->u32SrcFrameRate =
                pstRcAttr->stMjpegFixQp.u32SrcFrameRate;
            pOutAttr->fr32DstFrameRate =
                pstRcAttr->stMjpegFixQp.fr32DstFrameRate;
            s32Ret = CVI_SUCCESS;
        }
    } else if (pInChnAttr->stVencAttr.enType == PT_JPEG) {
        pOutAttr->picWidth = pstVencAttr->u32PicWidth;
        pOutAttr->picHeight = pstVencAttr->u32PicHeight;
        s32Ret = CVI_SUCCESS;
    }

    return s32Ret;
}

static CVI_S32
_cvi_venc_check_jpege_format(const VENC_MOD_JPEGE_S *pJpegeModParam)
{
    int i;
    unsigned int marker_cnt[JPEGE_MARKER_BUTT];
    const JPEGE_MARKER_TYPE_E *p = pJpegeModParam->JpegMarkerOrder;

    switch (pJpegeModParam->enJpegeFormat) {
    case JPEGE_FORMAT_DEFAULT:
    case JPEGE_FORMAT_TYPE_1:
        return CVI_SUCCESS;
    case JPEGE_FORMAT_CUSTOM:
        // proceed to check marker order validity
        break;
    default:
        CVI_VENC_ERR("Unknown JPEG format %d\n",
                 pJpegeModParam->enJpegeFormat);
        return CVI_ERR_VENC_JPEG_MARKER_ORDER;
    }

    memset(marker_cnt, 0, sizeof(marker_cnt));

    if (p[0] != JPEGE_MARKER_SOI) {
        CVI_VENC_ERR("The first jpeg marker must be SOI\n");
        return CVI_ERR_VENC_JPEG_MARKER_ORDER;
    }

    for (i = 0; i < JPEG_MARKER_ORDER_CNT; i++) {
        int type = p[i];
        if (JPEGE_MARKER_SOI <= type && type < JPEGE_MARKER_BUTT)
            marker_cnt[type] += 1;
        else
            break;
    }

    if (marker_cnt[JPEGE_MARKER_SOI] == 0) {
        CVI_VENC_ERR("There must be one SOI\n");
        return CVI_ERR_VENC_JPEG_MARKER_ORDER;
    }
    if (marker_cnt[JPEGE_MARKER_SOF0] == 0) {
        CVI_VENC_ERR("There must be one SOF0\n");
        return CVI_ERR_VENC_JPEG_MARKER_ORDER;
    }
    if (marker_cnt[JPEGE_MARKER_DQT] == 0 &&
        marker_cnt[JPEGE_MARKER_DQT_MERGE] == 0) {
        CVI_VENC_ERR("There must be one DQT or DQT_MERGE\n");
        return CVI_ERR_VENC_JPEG_MARKER_ORDER;
    }
    if (marker_cnt[JPEGE_MARKER_DQT] > 0 &&
        marker_cnt[JPEGE_MARKER_DQT_MERGE] > 0) {
        CVI_VENC_ERR("DQT and DQT_MERGE are mutually exclusive\n");
        return CVI_ERR_VENC_JPEG_MARKER_ORDER;
    }
    if (marker_cnt[JPEGE_MARKER_DHT] > 0 &&
        marker_cnt[JPEGE_MARKER_DHT_MERGE] > 0) {
        CVI_VENC_ERR("DHT and DHT_MERGE are mutually exclusive\n");
        return CVI_ERR_VENC_JPEG_MARKER_ORDER;
    }
    if (marker_cnt[JPEGE_MARKER_DRI] > 0 &&
        marker_cnt[JPEGE_MARKER_DRI_OPT] > 0) {
        CVI_VENC_ERR("DRI and DRI_OPT are mutually exclusive\n");
        return CVI_ERR_VENC_JPEG_MARKER_ORDER;
    }

    for (i = JPEGE_MARKER_SOI; i < JPEGE_MARKER_BUTT; i++) {
        if (marker_cnt[i] > 1) {
            CVI_VENC_ERR("Repeating marker type %d present\n", i);
            return CVI_ERR_VENC_JPEG_MARKER_ORDER;
        }
    }

    return CVI_SUCCESS;
}

static void _cvi_venc_config_jpege_format(VENC_MOD_JPEGE_S *pJpegeModParam)
{
    switch (pJpegeModParam->enJpegeFormat) {
    case JPEGE_FORMAT_DEFAULT:
        pJpegeModParam->JpegMarkerOrder[0] = JPEGE_MARKER_SOI;
        pJpegeModParam->JpegMarkerOrder[1] = JPEGE_MARKER_FRAME_INDEX;
        pJpegeModParam->JpegMarkerOrder[2] = JPEGE_MARKER_USER_DATA;
        pJpegeModParam->JpegMarkerOrder[3] = JPEGE_MARKER_DRI_OPT;
        pJpegeModParam->JpegMarkerOrder[4] = JPEGE_MARKER_DQT;
        pJpegeModParam->JpegMarkerOrder[5] = JPEGE_MARKER_DHT;
        pJpegeModParam->JpegMarkerOrder[6] = JPEGE_MARKER_SOF0;
        pJpegeModParam->JpegMarkerOrder[7] = JPEGE_MARKER_BUTT;
        return;
    case JPEGE_FORMAT_TYPE_1:
        pJpegeModParam->JpegMarkerOrder[0] = JPEGE_MARKER_SOI;
        pJpegeModParam->JpegMarkerOrder[1] = JPEGE_MARKER_JFIF;
        pJpegeModParam->JpegMarkerOrder[2] = JPEGE_MARKER_DQT_MERGE;
        pJpegeModParam->JpegMarkerOrder[3] = JPEGE_MARKER_SOF0;
        pJpegeModParam->JpegMarkerOrder[4] = JPEGE_MARKER_DHT_MERGE;
        pJpegeModParam->JpegMarkerOrder[5] = JPEGE_MARKER_DRI;
        pJpegeModParam->JpegMarkerOrder[6] = JPEGE_MARKER_BUTT;
        return;
    default:
        return;
    }
}

static CVI_VOID cviSetFps(venc_chn_context *pChnHandle, CVI_S32 currFps)
{
    CVI_FR32 fr32DstFrameRate = currFps & 0xFFFF;
    VENC_CHN_ATTR_S *pChnAttr;
    VENC_RC_ATTR_S *prcatt;

    pChnAttr = pChnHandle->pChnAttr;
    prcatt = &pChnAttr->stRcAttr;

    if (pChnAttr->stVencAttr.enType == PT_H264) {
        if (prcatt->enRcMode == VENC_RC_MODE_H264CBR) {
            prcatt->stH264Cbr.fr32DstFrameRate = fr32DstFrameRate;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H264VBR) {
            prcatt->stH264Vbr.fr32DstFrameRate = fr32DstFrameRate;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H264AVBR) {
            prcatt->stH264AVbr.fr32DstFrameRate = fr32DstFrameRate;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H264FIXQP) {
            prcatt->stH264FixQp.fr32DstFrameRate = fr32DstFrameRate;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H264UBR) {
            prcatt->stH264Ubr.fr32DstFrameRate = fr32DstFrameRate;
        }
    } else if (pChnAttr->stVencAttr.enType == PT_H265) {
        if (prcatt->enRcMode == VENC_RC_MODE_H265CBR) {
            prcatt->stH265Cbr.fr32DstFrameRate = fr32DstFrameRate;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265VBR) {
            prcatt->stH265Vbr.fr32DstFrameRate = fr32DstFrameRate;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265AVBR) {
            prcatt->stH265AVbr.fr32DstFrameRate = fr32DstFrameRate;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265FIXQP) {
            prcatt->stH265FixQp.fr32DstFrameRate = fr32DstFrameRate;
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265UBR) {
            prcatt->stH265Ubr.fr32DstFrameRate = fr32DstFrameRate;
        }
    }
}

static CVI_S32 cviCheckFps(venc_chn_context *pChnHandle,
               const VIDEO_FRAME_INFO_S *pstFrame)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;
    venc_vfps *pVfps = &pChnVars->vfps;
    const VIDEO_FRAME_S *pstVFrame = &pstFrame->stVFrame;
    CVI_S32 s32Ret = CVI_SUCCESS;
    CVI_S32 nextFps;

    if (pVfps->u64prevSec == 0) {
        pVfps->u64prevSec = pstVFrame->u64PTS;
    } else {
        if (pstVFrame->u64PTS - pVfps->u64prevSec >=
            pVfps->u64StatTime) {
            nextFps = pVfps->s32NumFrmsInOneSec /
                  CVI_DEF_VFPFS_STAT_TIME;

            cviSetFps(pChnHandle, nextFps);

            pVfps->u64prevSec = pstVFrame->u64PTS;
            pVfps->s32NumFrmsInOneSec = 0;

            s32Ret = CVI_ERR_VENC_STAT_VFPS_CHANGE;
        }
    }
    pVfps->s32NumFrmsInOneSec++;

    return s32Ret;
}


static CVI_S32 cviSetChnAttr(venc_chn_context *pChnHandle)
{
    cviVidChnAttr vidChnAttr = { 0 };
    cviJpegChnAttr jpgChnAttr = { 0 };
    CVI_S32 s32Ret = CVI_SUCCESS;
    VENC_CHN_ATTR_S *pChnAttr;
    VENC_ATTR_S *pVencAttr;
    venc_enc_ctx *pEncCtx = NULL;
    venc_chn_vars *pChnVars = NULL;

    pChnAttr = pChnHandle->pChnAttr;
    pChnVars = pChnHandle->pChnVars;
    pVencAttr = &pChnAttr->stVencAttr;
    pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }

    if (pVencAttr->enType == PT_H265 || pVencAttr->enType == PT_H264) {
        s32Ret = _cvi_h26x_trans_chn_attr(pChnHandle->pChnAttr,
                          &vidChnAttr);
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("h26x trans_chn_attr fail, %d\n", s32Ret);
            goto CHECK_RC_ATTR_RET;
        }

        s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_CHN_ATTR,
                         (CVI_VOID *)&vidChnAttr);
    } else if (pVencAttr->enType == PT_MJPEG ||
           pVencAttr->enType == PT_JPEG) {
        s32Ret = _cvi_jpg_trans_chn_attr(pChnHandle->pChnAttr,
                         &jpgChnAttr);
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("mjpeg trans_chn_attr fail, %d\n", s32Ret);
            goto CHECK_RC_ATTR_RET;
        }

        s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_JPEG_OP_SET_CHN_ATTR,
                         (CVI_VOID *)&jpgChnAttr);
    }

    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR(
            "CVI_H26X_OP_SET_CHN_ATTR or CVI_JPEG_OP_SET_CHN_ATTR, %d\n",
            s32Ret);
        goto CHECK_RC_ATTR_RET;
    }

    s32Ret = cviCheckRcModeAttr(pChnHandle); // RC side framerate control

CHECK_RC_ATTR_RET:
    pChnVars->bAttrChange = CVI_FALSE;
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("cviCheckRcModeAttr fail, %d\n", s32Ret);
        return s32Ret;
    }

    s32Ret = cviCheckGopAttr(pChnHandle);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("cviCheckGopAttr fail, %d\n", s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

static CVI_S32 cviCheckFrc(venc_chn_context *pChnHandle)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;
    venc_frc *pvf = &pChnVars->frc;
    CVI_S32 ifEncode = 1;

    if (pvf->bFrcEnable) {
        if (pvf->srcTs < pvf->dstTs) {
            ifEncode = 0;
        }
    }

    return ifEncode;
}

static CVI_S32 cviUpdateFrcDst(venc_chn_context *pChnHandle)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;
    venc_frc *pvf = &pChnVars->frc;

    if (pvf->bFrcEnable) {
        if (pvf->srcTs >= pvf->dstTs) {
            pvf->dstTs += pvf->dstFrameDur;
        }
    }

    return pvf->bFrcEnable;
}

static CVI_S32 cviUpdateFrcSrc(venc_chn_context *pChnHandle)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;
    venc_frc *pvf = &pChnVars->frc;

    if (pvf->bFrcEnable) {
        pvf->srcTs += pvf->srcFrameDur;
    }

    return pvf->bFrcEnable;
}

static CVI_VOID cviCheckFrcOverflow(venc_chn_context *pChnHandle)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;
    venc_frc *pvf = &pChnVars->frc;

    if (pvf->srcTs >= FRC_TIME_OVERFLOW_OFFSET &&
        pvf->dstTs >= FRC_TIME_OVERFLOW_OFFSET) {
        pvf->srcTs -= FRC_TIME_OVERFLOW_OFFSET;
        pvf->dstTs -= FRC_TIME_OVERFLOW_OFFSET;
    }
}


static inline CVI_S32 cviCheckVuiAspectRatio(
    const VENC_VUI_ASPECT_RATIO_S * pstVuiAspectRatio, CVI_S32 s32Err) {
    CVI_S32 s32Ret = CVI_SUCCESS;

    s32Ret = (pstVuiAspectRatio->aspect_ratio_info_present_flag <= 1) ? CVI_SUCCESS : s32Err;
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("error, %s = %d\n",
            "aspect_ratio_info_present_flag",
            pstVuiAspectRatio->aspect_ratio_info_present_flag);
        return s32Ret;
    }

    if (pstVuiAspectRatio->aspect_ratio_info_present_flag) {
        s32Ret = (pstVuiAspectRatio->sar_width >= CVI_H26X_SAR_WIDTH_MIN) ? CVI_SUCCESS : s32Err;
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("error, %s = %d\n", "sar_width", pstVuiAspectRatio->sar_width);
            return s32Ret;
        }

        s32Ret = (pstVuiAspectRatio->sar_height >= CVI_H26X_SAR_HEIGHT_MIN) ? CVI_SUCCESS : s32Err;
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("error, %s = %d\n", "sar_height", pstVuiAspectRatio->sar_height);
            return s32Ret;
        }
    }

    s32Ret = (pstVuiAspectRatio->overscan_info_present_flag <= 1) ? CVI_SUCCESS : s32Err;
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("error, %s = %d\n",
            "overscan_info_present_flag",
            pstVuiAspectRatio->overscan_info_present_flag);
        return s32Ret;
    }

    if (pstVuiAspectRatio->overscan_info_present_flag) {
        s32Ret = (pstVuiAspectRatio->overscan_appropriate_flag <= 1) ? CVI_SUCCESS : s32Err;
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("error, %s = %d\n",
                "overscan_appropriate_flag",
                pstVuiAspectRatio->overscan_appropriate_flag);
            return s32Ret;
        }
    }

    return s32Ret;
}

static inline CVI_S32 cviCheckVuiH264TimingInfo(
    const VENC_VUI_H264_TIME_INFO_S * pstVuiH264TimingInfo, CVI_S32 s32Err) {
    CVI_S32 s32Ret = CVI_SUCCESS;

    CVI_VENC_DBG(
        "timing_info_present_flag %u, fixed_frame_rate_flag %u\n",
        pstVuiH264TimingInfo->timing_info_present_flag,
        pstVuiH264TimingInfo->fixed_frame_rate_flag);
    CVI_VENC_DBG("num_units_in_tick %u, time_scale %u\n",
        pstVuiH264TimingInfo->num_units_in_tick,
        pstVuiH264TimingInfo->time_scale);

    if (pstVuiH264TimingInfo->timing_info_present_flag) {
        s32Ret = (pstVuiH264TimingInfo->fixed_frame_rate_flag <= 1) ? CVI_SUCCESS : s32Err;
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("error, %s = %d\n",
                "fixed_frame_rate_flag",
                pstVuiH264TimingInfo->fixed_frame_rate_flag);
            return s32Ret;
        }

        s32Ret = ((pstVuiH264TimingInfo->num_units_in_tick >= CVI_H26X_NUM_UNITS_IN_TICK_MIN) &&
            (pstVuiH264TimingInfo->num_units_in_tick <= CVI_H26X_NUM_UNITS_IN_TICK_MAX)) ?
            CVI_SUCCESS : s32Err;
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("error, %s = %d\n", "num_units_in_tick", pstVuiH264TimingInfo->num_units_in_tick);
            return s32Ret;
        }

        s32Ret = ((pstVuiH264TimingInfo->time_scale >= CVI_H26X_TIME_SCALE_MIN) &&
            (pstVuiH264TimingInfo->time_scale <= CVI_H26X_TIME_SCALE_MAX)) ? CVI_SUCCESS : s32Err;
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("error, %s = %d\n", "time_scale", pstVuiH264TimingInfo->time_scale);
            return s32Ret;
        }
    }

    return s32Ret;
}

static inline CVI_S32 cviCheckVuiH265TimingInfo(
    const VENC_VUI_H265_TIME_INFO_S * pstVuiH265TimingInfo, CVI_S32 s32Err) {
    CVI_S32 s32Ret = CVI_SUCCESS;

    CVI_VENC_DBG(
        "timing_info_present_flag %u, num_units_in_tick %u, time_scale %u\n",
        pstVuiH265TimingInfo->timing_info_present_flag,
        pstVuiH265TimingInfo->num_units_in_tick,
        pstVuiH265TimingInfo->time_scale);
    CVI_VENC_DBG("num_ticks_poc_diff_one_minus1 %u\n",
             pstVuiH265TimingInfo->num_ticks_poc_diff_one_minus1);

    s32Ret = (pstVuiH265TimingInfo->timing_info_present_flag <= 1) ? CVI_SUCCESS : s32Err;
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("error, %s = %d\n",
            "timing_info_present_flag",
            pstVuiH265TimingInfo->timing_info_present_flag);
        return s32Ret;
    }

    if (pstVuiH265TimingInfo->timing_info_present_flag) {
        s32Ret = ((pstVuiH265TimingInfo->num_units_in_tick >= CVI_H26X_NUM_UNITS_IN_TICK_MIN) &&
            (pstVuiH265TimingInfo->num_units_in_tick <= CVI_H26X_NUM_UNITS_IN_TICK_MAX)) ?
            CVI_SUCCESS : s32Err;
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("error, %s = %d\n", "num_units_in_tick", pstVuiH265TimingInfo->num_units_in_tick);
            return s32Ret;
        }

        s32Ret = ((pstVuiH265TimingInfo->time_scale >= CVI_H26X_TIME_SCALE_MIN) &&
            (pstVuiH265TimingInfo->time_scale <= CVI_H26X_TIME_SCALE_MAX)) ? CVI_SUCCESS : s32Err;
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("error, %s = %d\n", "time_scale", pstVuiH265TimingInfo->time_scale);
            return s32Ret;
        }

        s32Ret = (pstVuiH265TimingInfo->num_ticks_poc_diff_one_minus1
            <= CVI_H265_NUM_TICKS_POC_DIFF_ONE_MINUS1_MAX) ? CVI_SUCCESS : s32Err;
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("error, %s = %d\n",
                "num_ticks_poc_diff_one_minus1",
                pstVuiH265TimingInfo->num_ticks_poc_diff_one_minus1);
            return s32Ret;
        }
    }

    return s32Ret;
}

static inline CVI_S32 cviCheckVuiVideoSignal(
    const VENC_VUI_VIDEO_SIGNAL_S * pstVuiVideoSignal, CVI_U8 u8VideoFormatMax, CVI_S32 s32Err) {
    CVI_S32 s32Ret = CVI_SUCCESS;

    CVI_VENC_DBG(
        "video_signal_type_present_flag %u, video_format %d, video_full_range_flag %u\n",
        pstVuiVideoSignal->video_signal_type_present_flag,
        pstVuiVideoSignal->video_format,
        pstVuiVideoSignal->video_full_range_flag);
    CVI_VENC_DBG(
        "colour_description_present_flag %u, colour_primaries %u\n",
        pstVuiVideoSignal->colour_description_present_flag,
        pstVuiVideoSignal->colour_primaries);
    CVI_VENC_DBG(
        "transfer_characteristics %u, matrix_coefficients %u\n",
        pstVuiVideoSignal->transfer_characteristics,
        pstVuiVideoSignal->matrix_coefficients);

    s32Ret = (pstVuiVideoSignal->video_signal_type_present_flag <= 1) ? CVI_SUCCESS : s32Err;
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("error, %s = %d\n",
            "video_signal_type_present_flag",
            pstVuiVideoSignal->video_signal_type_present_flag);
        return s32Ret;
    }

    if (pstVuiVideoSignal->video_signal_type_present_flag) {
        s32Ret = (pstVuiVideoSignal->video_format <= u8VideoFormatMax) ? CVI_SUCCESS : s32Err;
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("error, %s = %d\n",
                "video_format", pstVuiVideoSignal->video_format);
            return s32Ret;
        }

        s32Ret = (pstVuiVideoSignal->video_full_range_flag <= 1) ? CVI_SUCCESS : s32Err;
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("error, %s = %d\n",
                "video_full_range_flag", pstVuiVideoSignal->video_full_range_flag);
            return s32Ret;
        }

        if (pstVuiVideoSignal->colour_description_present_flag) {
            s32Ret = (pstVuiVideoSignal->colour_description_present_flag <= 1) ? CVI_SUCCESS : s32Err;
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("error, %s = %d\n",
                    "colour_description_present_flag",
                    pstVuiVideoSignal->colour_description_present_flag);
                return s32Ret;
            }
        }
    }

    return s32Ret;
}

static inline CVI_S32 cviCheckBitstreamRestrict(
    const VENC_VUI_BITSTREAM_RESTRIC_S * pstBitstreamRestrict, CVI_S32 s32Err) {
    CVI_S32 s32Ret = CVI_SUCCESS;

    s32Ret = (pstBitstreamRestrict->bitstream_restriction_flag <= 1) ? CVI_SUCCESS : s32Err;
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("error, %s = %d\n",
            "bitstream_restriction_flag",
            pstBitstreamRestrict->bitstream_restriction_flag);
        return s32Ret;
    }

    return s32Ret;
}

static CVI_S32 _cviCheckH264VuiParam(const VENC_H264_VUI_S *pstH264Vui)
{
    CVI_S32 s32Ret = CVI_SUCCESS;

    s32Ret = cviCheckVuiAspectRatio(&pstH264Vui->stVuiAspectRatio,
                   CVI_ERR_VENC_H264_VUI);
    if (s32Ret != CVI_SUCCESS) {
        return s32Ret;
    }
    s32Ret = cviCheckVuiH264TimingInfo(&pstH264Vui->stVuiTimeInfo,
                   CVI_ERR_VENC_H264_VUI);
    if (s32Ret != CVI_SUCCESS) {
        return s32Ret;
    }
    s32Ret = cviCheckVuiVideoSignal(&pstH264Vui->stVuiVideoSignal,
                   CVI_H264_VIDEO_FORMAT_MAX,
                   CVI_ERR_VENC_H264_VUI);
    if (s32Ret != CVI_SUCCESS) {
        return s32Ret;
    }
    s32Ret = cviCheckBitstreamRestrict(&pstH264Vui->stVuiBitstreamRestric,
                    CVI_ERR_VENC_H264_VUI);
    if (s32Ret != CVI_SUCCESS) {
        return s32Ret;
    }
    return s32Ret;
}

static CVI_S32 _cviCheckH265VuiParam(const VENC_H265_VUI_S *pstH265Vui)
{
    CVI_S32 s32Ret = CVI_SUCCESS;

    s32Ret = cviCheckVuiAspectRatio(&pstH265Vui->stVuiAspectRatio,
                   CVI_ERR_VENC_H265_VUI);
    if (s32Ret != CVI_SUCCESS) {
        return s32Ret;
    }
    s32Ret = cviCheckVuiH265TimingInfo(&pstH265Vui->stVuiTimeInfo,
                   CVI_ERR_VENC_H265_VUI);
    if (s32Ret != CVI_SUCCESS) {
        return s32Ret;
    }
    s32Ret = cviCheckVuiVideoSignal(&pstH265Vui->stVuiVideoSignal,
                   CVI_H265_VIDEO_FORMAT_MAX,
                   CVI_ERR_VENC_H265_VUI);
    if (s32Ret != CVI_SUCCESS) {
        return s32Ret;
    }
    s32Ret = cviCheckBitstreamRestrict(&pstH265Vui->stVuiBitstreamRestric,
                    CVI_ERR_VENC_H265_VUI);
    if (s32Ret != CVI_SUCCESS) {
        return s32Ret;
    }
    return s32Ret;
}

static CVI_S32 _cviSetH264Vui(venc_chn_context *pChnHandle,
                  VENC_H264_VUI_S *pstH264Vui)
{
    CVI_S32 ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }
    ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_H264_VUI,
                  (CVI_VOID *)(pstH264Vui));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != CVI_SUCCESS)
        CVI_VENC_ERR("failed to set h264 vui %d", ret);

    return ret;
}

static CVI_S32 _cviSetH265Vui(venc_chn_context *pChnHandle,
                  VENC_H265_VUI_S *pstH265Vui)
{
    CVI_S32 ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }
    ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_H265_VUI,
                  (CVI_VOID *)(pstH265Vui));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != CVI_SUCCESS)
        CVI_VENC_ERR("failed to set h265 vui %d", ret);

    return ret;
}

#ifdef USB_VB_POOL
static CVI_S32 _cviVencUpdateVbConf(venc_chn_context *pChnHandle,
                    cviVbVencBufConfig *pVbVencCfg,
                    int VbSetFrameBufSize, int VbSetFrameBufCnt)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;

    #ifdef USE_VB_POOL
    int j = 0;
    for (j = 0; j < VbSetFrameBufCnt; j++) {
        CVI_U64 u64PhyAddr;

        pChnVars->vbBLK[j] =
            vb_get_block_with_id(pChnVars->vbpool.hPicVbPool,
                         VbSetFrameBufSize, CVI_ID_VENC);
        if (pChnVars->vbBLK[j] == VB_INVALID_HANDLE) {
            //not enough size in VB  or VB  pool not create
            CVI_VENC_ERR(
                "Not enough size in VB  or VB  pool Not create\n");
            return CVI_ERR_VENC_NOMEM;
        }

        u64PhyAddr = vb_handle2phys_addr(pChnVars->vbBLK[j]);
        pChnVars->FrmArray[j].phyAddr = u64PhyAddr;
        pChnVars->FrmArray[j].size = VbSetFrameBufSize;
        pChnVars->FrmArray[j].virtAddr =
            vb_handle2VirtAddr(pChnVars->vbBLK[j]);
        pVbVencCfg->vbModeAddr[j] = u64PhyAddr;
    }
    #endif

    pChnVars->FrmNum = VbSetFrameBufCnt;
    pVbVencCfg->VbSetFrameBufCnt = VbSetFrameBufCnt;
    pVbVencCfg->VbSetFrameBufSize = VbSetFrameBufSize;

    return CVI_SUCCESS;
}
#endif

static CVI_S32 _cviVencAllocVbBuf(VENC_CHN VeChn)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_context *pVencHandle = handle;
    venc_chn_context *pChnHandle = NULL;
    VENC_CHN_ATTR_S *pChnAttr = NULL;
    VENC_ATTR_S *pVencAttr = NULL;
    venc_enc_ctx *pEncCtx = NULL;
    venc_chn_vars *pChnVars = NULL;
    VB_SOURCE_E eVbSource;
    cviVbVencBufConfig cviVbVencCfg;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    if (pVencHandle == NULL) {
        CVI_VENC_ERR(
            "p_venctx_handle NULL (Channel not create yet..)\n");
        return CVI_ERR_VENC_NULL_PTR;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnAttr = pChnHandle->pChnAttr;
    pChnVars = pChnHandle->pChnVars;
    pEncCtx = &pChnHandle->encCtx;
    pVencAttr = &pChnAttr->stVencAttr;

    if (pVencAttr->enType == PT_H264) {
        eVbSource =
            pVencHandle->ModParam.stH264eModParam.enH264eVBSource;
    } else if (pVencAttr->enType == PT_H265) {
        eVbSource =
            pVencHandle->ModParam.stH265eModParam.enH265eVBSource;
    } else {
        eVbSource = VB_SOURCE_COMMON;
    }

    memset(&cviVbVencCfg, 0, sizeof(cviVbVencBufConfig));
    cviVbVencCfg.VbType = VB_SOURCE_COMMON;

    if (eVbSource == VB_SOURCE_PRIVATE) {
        if (pChnVars->bHasVbPool == CVI_FALSE) {
            // struct cvi_vb_pool_cfg stVbPoolCfg;

            pr_err("error warning,vb pool has not implemented\n");
            #ifdef USB_VB_POOL
            //step1 get vb info from vpu driver: calculate the buffersize and buffer count
            s32Ret = pEncCtx->base.ioctl(pEncCtx,
                             CVI_H26X_OP_GET_VB_INFO,
                             (CVI_VOID *)&cviVbVencCfg);

            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("CVI_H26X_OP_GET_VB_INFO, %d\n",
                         s32Ret);
                return s32Ret;
            }

            stVbPoolCfg.blk_size = cviVbVencCfg.VbGetFrameBufSize;
            stVbPoolCfg.blk_cnt = cviVbVencCfg.VbGetFrameBufCnt;
            stVbPoolCfg.remap_mode = VB_REMAP_MODE_NONE;
            if (vb_create_pool(&stVbPoolCfg) == 0) {
                pChnVars->vbpool.hPicVbPool =
                    stVbPoolCfg.pool_id;
            }
            pChnVars->bHasVbPool = CVI_TRUE;
            s32Ret = _cviVencUpdateVbConf(
                pChnHandle, &cviVbVencCfg,
                cviVbVencCfg.VbGetFrameBufSize,
                cviVbVencCfg.VbGetFrameBufCnt);
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("_cviVencUpdateVbConf, %d\n",
                         s32Ret);
                return s32Ret;
            }
            #endif
            cviVbVencCfg.VbType = VB_SOURCE_PRIVATE;
        }
    } else if (eVbSource == VB_SOURCE_USER) {
    #ifdef USE_VB_POOL
        struct cvi_vb_cfg pstVbConfig;
        CVI_S32 s32UserSetupFrmCnt;
        CVI_S32 s32UserSetupFrmSize;
        //step1 get vb info from vpu driver: calculate the buffersize and buffer count
        s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_GET_VB_INFO,
                         (CVI_VOID *)&cviVbVencCfg);
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("CVI_H26X_OP_GET_VB_INFO, %d\n", s32Ret);
            return s32Ret;
        }

        //totally setup by user vbpool id, if not been attachVB return failure
        if (pChnVars->bHasVbPool == CVI_FALSE) {
            CVI_VENC_ERR("[error][%s][%d]\n", __func__, __LINE__);
            CVI_VENC_ERR("Not attach vb pool for channel[%d]\n",
                     VeChn);
            return CVI_ERR_VENC_NOT_SUPPORT;
        }
//check the VB config and compare with VPU requirement
        s32Ret = vb_get_config(&pstVbConfig);
        if (s32Ret == CVI_SUCCESS) {
            if ((int)pstVbConfig
                    .comm_pool[pChnVars->vbpool.hPicVbPool]
                    .blk_size <
                cviVbVencCfg.VbGetFrameBufSize) {
                CVI_VENC_WARN(
                    "[Warning] create size is smaller than require size\n");
            }
            if ((int)pstVbConfig
                    .comm_pool[pChnVars->vbpool.hPicVbPool]
                    .blk_size < cviVbVencCfg.VbGetFrameBufCnt) {
                CVI_VENC_WARN(
                    "[Warning] create blk is smaller than require blk\n");
            }
        } else
            CVI_VENC_ERR("Error while CVI_VB_GetConfig\n");

        s32UserSetupFrmSize =
            pstVbConfig.comm_pool[pChnVars->vbpool.hPicVbPool]
                .blk_size;
        //s32UserSetupFrmCnt = pstVbConfig.comm_pool[pChnHandle->vbpool.hPicVbPool].u32BlkCnt;
        if (s32UserSetupFrmSize < cviVbVencCfg.VbGetFrameBufSize) {
            CVI_VENC_WARN(
                "Buffer size too small for frame buffer : user mode VB pool[%d] < [%d]n",
                pstVbConfig
                    .comm_pool[pChnVars->vbpool.hPicVbPool]
                    .blk_size,
                cviVbVencCfg.VbGetFrameBufSize);
        }

        s32UserSetupFrmCnt = cviVbVencCfg.VbGetFrameBufCnt;
        s32UserSetupFrmSize = cviVbVencCfg.VbGetFrameBufSize;
        s32Ret = _cviVencUpdateVbConf(pChnHandle, &cviVbVencCfg,
                          s32UserSetupFrmSize,
                          s32UserSetupFrmCnt);
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("_cviVencUpdateVbConf, %d\n", s32Ret);
            return s32Ret;
        }
        #endif
        cviVbVencCfg.VbType = VB_SOURCE_USER;
    }

    if (pEncCtx->base.ioctl) {
        s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_VB_BUFFER,
                         (CVI_VOID *)&cviVbVencCfg);
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("CVI_H26X_OP_SET_VB_BUFFER, %d\n", s32Ret);
            return s32Ret;
        }
    }

    return s32Ret;
}


static CVI_S32 cviInitChnCtx(VENC_CHN VeChn, const VENC_CHN_ATTR_S *pstAttr)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;

    handle->chn_handle[VeChn] = MEM_CALLOC(1, sizeof(venc_chn_context));
    if (handle->chn_handle[VeChn] == NULL) {
        CVI_VENC_ERR("Allocate chn_handle memory failed !\n");
        s32Ret = CVI_ERR_VENC_NOMEM;
        goto ERR_CVI_INIT_CHN_CTX;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnHandle->VeChn = VeChn;

    MUTEX_INIT(&pChnHandle->chnMutex, 0);
    MUTEX_INIT(&pChnHandle->chnShmMutex, &ma);

    pChnHandle->pChnAttr = MEM_MALLOC(sizeof(VENC_CHN_ATTR_S));
    if (pChnHandle->pChnAttr == NULL) {
        CVI_VENC_ERR("Allocate pChnAttr memory failed !\n");
        s32Ret = CVI_ERR_VENC_NOMEM;
        goto ERR_CVI_INIT_CHN_CTX_1;
    }

    memcpy(pChnHandle->pChnAttr, pstAttr, sizeof(VENC_CHN_ATTR_S));

    pChnHandle->pChnVars = MEM_CALLOC(1, sizeof(venc_chn_vars));
    if (pChnHandle->pChnVars == NULL) {
        CVI_VENC_ERR("Allocate pChnVars memory failed !\n");
        s32Ret = CVI_ERR_VENC_NOMEM;
        goto ERR_CVI_INIT_CHN_CTX_2;
    }

    pChnHandle->pChnVars->chnState = VENC_CHN_STATE_INIT;

    pChnHandle->pVbCtx = &vencVbCtx[VeChn];

    pEncCtx = &pChnHandle->encCtx;
    if (venc_create_enc_ctx(pEncCtx, pChnHandle) < 0) {
        CVI_VENC_ERR("venc_create_enc_ctx\n");
        s32Ret = CVI_ERR_VENC_NOMEM;
        goto ERR_CVI_INIT_CHN_CTX_3;
    }

    s32Ret = pEncCtx->base.init();
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("base init,s32Ret %#x\n", s32Ret);
        goto ERR_CVI_INIT_CHN_CTX_3;
    }

    s32Ret = cviCheckRcModeAttr(pChnHandle);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("cviCheckRcModeAttr\n");
        goto ERR_CVI_INIT_CHN_CTX_3;
    }

    s32Ret = cviCheckGopAttr(pChnHandle);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("cviCheckGopAttr\n");
        goto ERR_CVI_INIT_CHN_CTX_3;
    }

    s32Ret = cviCheckGopExAttr(pChnHandle);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("cviCheckGopExAttr\n");
        goto ERR_CVI_INIT_CHN_CTX_3;
    }

    s32Ret = cviSetChnDefault(pChnHandle);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("cviSetChnDefault,s32Ret %#x\n", s32Ret);
        goto ERR_CVI_INIT_CHN_CTX_3;
    }

    return s32Ret;

ERR_CVI_INIT_CHN_CTX_3:
    if (pChnHandle->pChnVars) {
        MEM_FREE(pChnHandle->pChnVars);
        pChnHandle->pChnVars = NULL;
    }
ERR_CVI_INIT_CHN_CTX_2:
    if (pChnHandle->pChnAttr) {
        MEM_FREE(pChnHandle->pChnAttr);
        pChnHandle->pChnAttr = NULL;
    }
ERR_CVI_INIT_CHN_CTX_1:
    if (handle->chn_handle[VeChn]) {
        MEM_FREE(handle->chn_handle[VeChn]);
        handle->chn_handle[VeChn] = NULL;
    }
ERR_CVI_INIT_CHN_CTX:

    return s32Ret;
}

static CVI_S32 cviCheckLeftStreamFrames(venc_chn_context *pChnHandle)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;
    VENC_CHN_STATUS_S *pChnStat = &pChnVars->chnStatus;
    CVI_S32 s32Ret = CVI_SUCCESS;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }
    if (pChnStat->u32LeftStreamFrames <= 0) {
        CVI_VENC_WARN(
            "u32LeftStreamFrames <= 0, no stream data to get\n");
        s32Ret = CVI_ERR_VENC_EMPTY_STREAM_FRAME;
    }
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    return s32Ret;
}

static CVI_S32 updateStreamPackInBindMode(venc_chn_context *pChnHandle,
                      venc_chn_vars *pChnVars,
                      VENC_STREAM_S *pstStream)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    CVI_U32 idx = 0;

    if (pChnVars->s32BindModeGetStreamRet != CVI_SUCCESS) {
        CVI_VENC_ERR("bind mode get stream error: 0x%X\n",
                 pChnVars->s32BindModeGetStreamRet);
        return pChnVars->s32BindModeGetStreamRet;
    }

    if (!pChnVars->stStream.pstPack) {
        CVI_VENC_ERR("pChnVars->stStream.pstPack is NULL\n");
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    s32Ret = cviCheckLeftStreamFrames(pChnHandle);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_WARN("cviCheckLeftStreamFrames, s32Ret = 0x%X\n",
                  s32Ret);
        return s32Ret;
    }

    pstStream->u32PackCount = pChnVars->stStream.u32PackCount;
    for (idx = 0; idx < pstStream->u32PackCount; idx++) {
        VENC_PACK_S *ppack = &pstStream->pstPack[idx];

        memcpy(ppack, &pChnVars->stStream.pstPack[idx],
               sizeof(VENC_PACK_S));
        ppack->u64PTS = pChnVars->currPTS;
    }

    return s32Ret;
}

static CVI_S32 cviVenc_sem_timedwait_Millsecs(struct semaphore *sem, long msecs)
{
    int ret;
    CVI_S32 s32RetStatus;

    if (msecs < 0)
        ret = SEMA_WAIT(sem);
    else if (msecs == 0)
        ret = SEMA_TRYWAIT(sem);
    else
        ret = SEMA_TIMEWAIT(sem, usecs_to_jiffies(msecs * 1000));

    if (ret == 0) {
        s32RetStatus = CVI_SUCCESS;
    } else {
        if (ret > 0) {
            s32RetStatus = CVI_ERR_VENC_BUSY;
        } else if (ret == -EINTR) {
            s32RetStatus = CVI_FAILURE;
        } else if (ret == -ETIME) {
            s32RetStatus = CVI_ERR_VENC_BUSY;
        } else {
            CVI_VENC_ERR("sem unexpected errno[%d]time[%ld]\n", ret,
                     msecs);
            s32RetStatus = CVI_FAILURE;
        }
    }
    return s32RetStatus;
}


CVI_S32 cviGetLeftStreamFrames(CVI_S32 VeChn)
{
    venc_chn_context *pChnHandle = NULL;
    venc_chn_vars *pChnVars = NULL;
    VENC_CHN_STATUS_S *pChnStat = NULL;

    if (handle == NULL) {
        return -1;
    }

    if (handle->chn_handle[VeChn] == NULL) {
        return -2;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;
    pChnStat = &pChnVars->chnStatus;

    return pChnStat->u32LeftStreamFrames;
}

static CVI_S32 cviSetVencPerfAttrToProc(venc_chn_context *pChnHandle)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_vars *pChnVars = NULL;
    venc_enc_ctx *pEncCtx = NULL;
    CVI_U64 u64CurTime = get_current_time();

    pChnVars = pChnHandle->pChnVars;
    pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnShmMutex) != 0) {
        CVI_VENC_ERR("can not lock chnShmMutex\n");
        return CVI_FAILURE;
    }
    if ((u64CurTime - pChnVars->u64LastGetStreamTimeStamp) > SEC_TO_MS) {
        pChnVars->stFPS.u32OutFPS =
            (CVI_U32)((pChnVars->u32GetStreamCnt * SEC_TO_MS) /
                  ((CVI_U32)(u64CurTime -
                         pChnVars->u64LastGetStreamTimeStamp)));
        pChnVars->u64LastGetStreamTimeStamp = u64CurTime;
        pChnVars->u32GetStreamCnt = 0;
    }
    pChnVars->stFPS.u64MaxHwTime = MAX(pChnVars->stFPS.u64MaxHwTime, pEncCtx->base.u64EncHwTime);
    pChnVars->stFPS.u64HwTime = pEncCtx->base.u64EncHwTime;
    MUTEX_UNLOCK(&pChnHandle->chnShmMutex);

    return s32Ret;
}

static CVI_S32 cviProcessResult(venc_chn_context *pChnHandle,
                VENC_STREAM_S *pstStream)
{
    venc_chn_vars *pChnVars = NULL;
    VENC_CHN_ATTR_S *pChnAttr;
    VENC_ATTR_S *pVencAttr;
    CVI_S32 s32Ret = CVI_SUCCESS;

    pChnVars = pChnHandle->pChnVars;
    pChnAttr = pChnHandle->pChnAttr;
    pVencAttr = &pChnAttr->stVencAttr;

    pChnVars->u32GetStreamCnt += pstStream->u32PackCount;
    s32Ret = cviSetVencPerfAttrToProc(pChnHandle);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("(chn %d) cviSetVencPerfAttrToProc fail\n",
                 pChnHandle->VeChn);
        return s32Ret;
    }

    if (pVencAttr->enType == PT_JPEG || pVencAttr->enType == PT_MJPEG) {
        if (pstStream->u32PackCount > 0) {
            VENC_PACK_S *ppack = &pstStream->pstPack[pstStream->u32PackCount -1];
            int j;

            for (j = (ppack->u32Len - ppack->u32Offset - 1); j > 0;
                 j--) {
                unsigned char *tmp_ptr =
                    ppack->pu8Addr + ppack->u32Offset + j;
                if (tmp_ptr[0] == 0xd9 && tmp_ptr[-1] == 0xff) {
                    break;
                }
            }
            ppack->u32Len = ppack->u32Offset + j + 1;
        }
    }

    return s32Ret;
}

static CVI_VOID cviUpdateChnStatus(venc_chn_context *pChnHandle)
{
    venc_chn_vars *pChnVars = pChnHandle->pChnVars;
    VENC_CHN_STATUS_S *pChnStat = &pChnVars->chnStatus;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return;
    }

    pChnStat->stVencStrmInfo.u32MeanQp =
        pEncCtx->ext.vid.streamInfo.u32MeanQp;
    pChnStat->u32LeftStreamFrames--;

    MUTEX_UNLOCK(&pChnHandle->chnMutex);
}


static CVI_S32 cviSetVencChnAttrToProc(VENC_CHN VeChn,
                       const VIDEO_FRAME_INFO_S *pstFrame)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_vars *pChnVars = NULL;
    venc_chn_context *pChnHandle = NULL;
    CVI_U64 u64CurTime = get_current_time();


    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    if (pstFrame == NULL) {
        CVI_VENC_ERR("pstFrame is NULL\n");
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;

    if (MUTEX_LOCK(&pChnHandle->chnShmMutex) != 0) {
        CVI_VENC_ERR("can not lock chnShmMutex\n");
        return CVI_FAILURE;
    }
    memcpy(&pChnVars->stFrameInfo, pstFrame, sizeof(VIDEO_FRAME_INFO_S));
    if ((u64CurTime - pChnVars->u64LastSendFrameTimeStamp) > SEC_TO_MS) {
        pChnVars->stFPS.u32InFPS =
            (CVI_U32)((pChnVars->u32SendFrameCnt * SEC_TO_MS) /
                  (CVI_U32)(u64CurTime -
                        pChnVars->u64LastSendFrameTimeStamp));
        pChnVars->u64LastSendFrameTimeStamp = u64CurTime;
        pChnVars->u32SendFrameCnt = 0;
    }
    MUTEX_UNLOCK(&pChnHandle->chnShmMutex);

    return s32Ret;
}

static CVI_S32 cviVencH265SetTrans(venc_chn_context *pChnHandle,
                   VENC_H265_TRANS_S *pstH265Trans)
{
    CVI_S32 ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }
    ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_H265_TRANS,
                  (CVI_VOID *)(pstH265Trans));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != CVI_SUCCESS)
        CVI_VENC_ERR("failed to set h265 trans %d", ret);

    return ret;
}

static CVI_S32 cviCheckRcParam(venc_chn_context *pChnHandle,
                   const VENC_RC_PARAM_S *pstRcParam)
{
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    VENC_ATTR_S *pVencAttr = &pChnAttr->stVencAttr;
    VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
    CVI_S32 s32Ret = CVI_SUCCESS;

    s32Ret = ((pstRcParam->s32InitialDelay >= CVI_INITIAL_DELAY_MIN) &&
        (pstRcParam->s32InitialDelay <= CVI_INITIAL_DELAY_MAX)) ?
        CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("error, %s = %d\n",
            "s32InitialDelay", pstRcParam->s32InitialDelay);
        return s32Ret;
    }

    if (pVencAttr->enType == PT_H264) {
        if (prcatt->enRcMode == VENC_RC_MODE_H264CBR) {
            s32Ret = cviCheckCommonRCParam(pstRcParam, pVencAttr, prcatt->enRcMode);
            if (s32Ret != CVI_SUCCESS) {
                return s32Ret;
            }
        } else if (prcatt->enRcMode == VENC_RC_MODE_H264VBR) {
            const VENC_PARAM_H264_VBR_S *pprc =
                &pstRcParam->stParamH264Vbr;

            s32Ret = cviCheckCommonRCParam(pstRcParam, pVencAttr, prcatt->enRcMode);
            if (s32Ret != CVI_SUCCESS) {
                return s32Ret;
            }
            s32Ret = ((pprc->s32ChangePos >= CVI_H26X_CHANGE_POS_MIN) &&
                (pprc->s32ChangePos <= CVI_H26X_CHANGE_POS_MAX)) ?
                CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("error, %s = %d\n",
                    "s32ChangePos", pprc->s32ChangePos);
                return s32Ret;
            }
        } else if (prcatt->enRcMode == VENC_RC_MODE_H264AVBR) {
            const VENC_PARAM_H264_AVBR_S *pprc =
                &pstRcParam->stParamH264AVbr;

            s32Ret = cviCheckCommonRCParam(pstRcParam, pVencAttr, prcatt->enRcMode);
            if (s32Ret != CVI_SUCCESS) {
                return s32Ret;
            }
            s32Ret = ((pprc->s32ChangePos >= CVI_H26X_CHANGE_POS_MIN) &&
                (pprc->s32ChangePos <= CVI_H26X_CHANGE_POS_MAX)) ?
                CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("error, %s = %d\n",
                    "s32ChangePos", pprc->s32ChangePos);
                return s32Ret;
            }
            s32Ret = ((pprc->s32MinStillPercent >= CVI_H26X_MIN_STILL_PERCENT_MIN) &&
                (pprc->s32MinStillPercent <= CVI_H26X_MIN_STILL_PERCENT_MAX)) ?
                CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("error, %s = %d\n",
                    "s32MinStillPercent", pprc->s32MinStillPercent);
                return s32Ret;
            }
            s32Ret = ((pprc->u32MaxStillQP >= pprc->u32MinQp) &&
                (pprc->u32MaxStillQP <= pprc->u32MaxQp)) ?
                CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("error, %s = %d\n",
                    "u32MaxStillQP", pprc->u32MaxStillQP);
                return s32Ret;
            }
            s32Ret = ((pprc->u32MotionSensitivity >= CVI_H26X_MOTION_SENSITIVITY_MIN) &&
                (pprc->u32MotionSensitivity <= CVI_H26X_MOTION_SENSITIVITY_MAX)) ?
                CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("error, %s = %d\n",
                    "u32MotionSensitivity", pprc->u32MotionSensitivity);
                return s32Ret;
            }
            s32Ret = ((pprc->s32AvbrFrmLostOpen >= CVI_H26X_AVBR_FRM_LOST_OPEN_MIN) &&
                (pprc->s32AvbrFrmLostOpen <= CVI_H26X_AVBR_FRM_LOST_OPEN_MAX)) ?
                CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("error, %s = %d\n",
                    "s32AvbrFrmLostOpen", pprc->s32AvbrFrmLostOpen);
                return s32Ret;
            }
            s32Ret = ((pprc->s32AvbrFrmGap >= CVI_H26X_AVBR_FRM_GAP_MIN) &&
                (pprc->s32AvbrFrmGap <= CVI_H26X_AVBR_FRM_GAP_MAX)) ?
                CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("error, %s = %d\n",
                    "s32AvbrFrmGap", pprc->s32AvbrFrmGap);
                return s32Ret;
            }
            s32Ret = ((pprc->s32AvbrPureStillThr >= CVI_H26X_AVBR_PURE_STILL_THR_MIN) &&
                (pprc->s32AvbrPureStillThr <= CVI_H26X_AVBR_PURE_STILL_THR_MAX)) ?
                CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("error, %s = %d\n",
                    "s32AvbrPureStillThr", pprc->s32AvbrPureStillThr);
                return s32Ret;
            }
        } else if (prcatt->enRcMode == VENC_RC_MODE_H264UBR) {
            s32Ret = cviCheckCommonRCParam(pstRcParam, pVencAttr, prcatt->enRcMode);
            if (s32Ret != CVI_SUCCESS) {
                return s32Ret;
            }
        }
    } else if (pVencAttr->enType == PT_H265) {
        if (prcatt->enRcMode == VENC_RC_MODE_H265CBR) {
            s32Ret = cviCheckCommonRCParam(pstRcParam, pVencAttr, prcatt->enRcMode);
            if (s32Ret != CVI_SUCCESS) {
                return s32Ret;
            }
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265VBR) {
            const VENC_PARAM_H265_VBR_S *pprc =
                &pstRcParam->stParamH265Vbr;

            s32Ret = cviCheckCommonRCParam(pstRcParam, pVencAttr, prcatt->enRcMode);
            if (s32Ret != CVI_SUCCESS) {
                return s32Ret;
            }
            s32Ret = ((pprc->s32ChangePos >= CVI_H26X_CHANGE_POS_MIN) &&
                (pprc->s32ChangePos <= CVI_H26X_CHANGE_POS_MAX)) ?
                CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("error, %s = %d\n",
                    "s32ChangePos", pprc->s32ChangePos);
                return s32Ret;
            }
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265AVBR) {
            const VENC_PARAM_H265_AVBR_S *pprc =
                &pstRcParam->stParamH265AVbr;

            s32Ret = cviCheckCommonRCParam(pstRcParam, pVencAttr, prcatt->enRcMode);
            if (s32Ret != CVI_SUCCESS) {
                return s32Ret;
            }
            s32Ret = ((pprc->s32ChangePos >= CVI_H26X_CHANGE_POS_MIN) &&
                (pprc->s32ChangePos <= CVI_H26X_CHANGE_POS_MAX)) ?
                CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("error, %s = %d\n",
                    "s32ChangePos", pprc->s32ChangePos);
                return s32Ret;
            }
            s32Ret = ((pprc->s32MinStillPercent >= CVI_H26X_MIN_STILL_PERCENT_MIN) &&
                (pprc->s32MinStillPercent <= CVI_H26X_MIN_STILL_PERCENT_MAX)) ?
                CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("error, %s = %d\n",
                    "s32MinStillPercent", pprc->s32MinStillPercent);
                return s32Ret;
            }
            s32Ret = ((pprc->u32MaxStillQP >= pprc->u32MinQp) &&
                (pprc->u32MaxStillQP <= pprc->u32MaxQp)) ?
                CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("error, %s = %d\n",
                    "u32MaxStillQP", pprc->u32MaxStillQP);
                return s32Ret;
            }
            s32Ret = ((pprc->u32MotionSensitivity >= CVI_H26X_MOTION_SENSITIVITY_MIN) &&
                (pprc->u32MotionSensitivity <= CVI_H26X_MOTION_SENSITIVITY_MAX)) ?
                CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("error, %s = %d\n",
                    "u32MotionSensitivity", pprc->u32MotionSensitivity);
                return s32Ret;
            }
            s32Ret = ((pprc->s32AvbrFrmLostOpen >= CVI_H26X_AVBR_FRM_LOST_OPEN_MIN) &&
                (pprc->s32AvbrFrmLostOpen <= CVI_H26X_AVBR_FRM_LOST_OPEN_MAX)) ?
                CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("error, %s = %d\n",
                    "s32AvbrFrmLostOpen", pprc->s32AvbrFrmLostOpen);
                return s32Ret;
            }
            s32Ret = ((pprc->s32AvbrFrmGap >= CVI_H26X_AVBR_FRM_GAP_MIN) &&
                (pprc->s32AvbrFrmGap <= CVI_H26X_AVBR_FRM_GAP_MAX)) ?
                CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("error, %s = %d\n",
                    "s32AvbrFrmGap", pprc->s32AvbrFrmGap);
                return s32Ret;
            }
            s32Ret = ((pprc->s32AvbrPureStillThr >= CVI_H26X_AVBR_PURE_STILL_THR_MIN) &&
                (pprc->s32AvbrPureStillThr <= CVI_H26X_AVBR_PURE_STILL_THR_MAX)) ?
                CVI_SUCCESS : CVI_ERR_VENC_RC_PARAM;
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("error, %s = %d\n",
                    "s32AvbrPureStillThr", pprc->s32AvbrPureStillThr);
                return s32Ret;
            }
        } else if (prcatt->enRcMode == VENC_RC_MODE_H265UBR) {
            s32Ret = cviCheckCommonRCParam(pstRcParam, pVencAttr, prcatt->enRcMode);
            if (s32Ret != CVI_SUCCESS) {
                return s32Ret;
            }
        }
    }

    return s32Ret;
}


static CVI_S32 cviSetCuPredToDrv(venc_chn_context *pChnHandle)
{
    VENC_CU_PREDICTION_S *pcup = &pChnHandle->pChnVars->cuPrediction;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;
    cviPred pred, *pPred = &pred;
    CVI_S32 s32Ret = CVI_SUCCESS;

    pPred->u32IntraCost = pcup->u32IntraCost;

    s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_PREDICT,
                     (CVI_VOID *)pPred);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("CVI_H26X_OP_SET_PREDICT, %d\n", s32Ret);
        return CVI_ERR_VENC_CU_PREDICTION;
    }

    return s32Ret;
}


static CVI_S32 cviVencJpegInsertUserData(venc_chn_context *pChnHandle,
                     CVI_U8 *pu8Data, CVI_U32 u32Len)
{
    CVI_S32 ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;
    cviJpegUserData stUserData;

    stUserData.userData = pu8Data;
    stUserData.len = u32Len;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }
    ret = pEncCtx->base.ioctl(pEncCtx, CVI_JPEG_OP_SET_USER_DATA,
                  (CVI_VOID *)(&stUserData));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != CVI_SUCCESS)
        CVI_VENC_ERR("jpeg failed to insert user data, %d", ret);
    return ret;
}

static CVI_S32 cviVencH26xInsertUserData(venc_chn_context *pChnHandle,
                     CVI_U8 *pu8Data, CVI_U32 u32Len)
{
    CVI_S32 ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;
    cviUserData stUserData;

    stUserData.userData = pu8Data;
    stUserData.len = u32Len;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }
    ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_USER_DATA,
                  (CVI_VOID *)(&stUserData));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != CVI_SUCCESS)
        CVI_VENC_ERR("h26x failed to insert user data, %d", ret);
    return ret;
}

static CVI_S32 cviVencH264SetEntropy(venc_chn_context *pChnHandle,
                     VENC_H264_ENTROPY_S *pstH264Entropy)
{
    CVI_S32 ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }
    ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_H264_ENTROPY,
                  (CVI_VOID *)(pstH264Entropy));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != CVI_SUCCESS)
        CVI_VENC_ERR("failed to set h264 entropy, %d", ret);

    return ret;
}

static CVI_S32 cviVencH265SetPredUnit(venc_chn_context *pChnHandle,
        VENC_H265_PU_S *pstH265PredUnit)
{
    CVI_S32 ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }

    ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_H265_PRED_UNIT,
           (CVI_VOID *)(pstH265PredUnit));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != CVI_SUCCESS)
        CVI_VENC_ERR("failed to set h265 PredUnit %d", ret);

    return ret;
}

static CVI_S32 cviVencH265GetPredUnit(venc_chn_context *pChnHandle,
        VENC_H265_PU_S *pstH265PredUnit)
{
    CVI_S32 ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }

    ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_GET_H265_PRED_UNIT,
           (CVI_VOID *)(pstH265PredUnit));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != CVI_SUCCESS)
        CVI_VENC_ERR("failed to get h265 PredUnit %d", ret);

    return ret;
}

static CVI_S32 cviVencH264SetTrans(venc_chn_context *pChnHandle,
                   VENC_H264_TRANS_S *pstH264Trans)
{
    CVI_S32 ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }
    ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_H264_TRANS,
                  (CVI_VOID *)(pstH264Trans));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != CVI_SUCCESS)
        CVI_VENC_ERR("failed to set h264 trans %d", ret);

    return ret;
}

static CVI_S32 cviVencSetSearchWindow(venc_chn_context *pChnHandle, VENC_SEARCH_WINDOW_S *pstVencSearchWindow)
{
    CVI_S32 ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }

    ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_SEARCH_WINDOW,
           (CVI_VOID *)(pstVencSearchWindow));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != CVI_SUCCESS)
        CVI_VENC_ERR("failed to set search window %d", ret);

    return ret;
}

static CVI_S32 cviVencGetSearchWindow(venc_chn_context *pChnHandle, VENC_SEARCH_WINDOW_S *pstVencSearchWindow)
{
    CVI_S32 ret;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }

    ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_GET_SEARCH_WINDOW,
           (CVI_VOID *)(pstVencSearchWindow));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (ret != CVI_SUCCESS)
        CVI_VENC_ERR("failed to get search window %d", ret);

    return ret;
}
/**
 * @brief Create One VENC Channel.
 * @param[in] VeChn VENC Channel Number.
 * @param[in] pstAttr pointer to VENC_CHN_ATTR_S.
 * @retval 0 Success
 * @retval Non-0 Failure, please see the error code table
 */
CVI_S32 CVI_VENC_CreateChn(VENC_CHN VeChn, const VENC_CHN_ATTR_S *pstAttr)
{

    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    venc_enc_ctx *pEncCtx = NULL;

    s32Ret = cviInitChnCtx(VeChn, pstAttr);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("init\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;

    s32Ret = pEncCtx->base.open(handle, pChnHandle);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("venc_init_encoder\n");
        return s32Ret;
    }

    pChnHandle->bSbSkipFrm = false;
    handle->chn_status[VeChn] = VENC_CHN_STATE_INIT;
    return s32Ret;
}


CVI_S32 CVI_VENC_StartRecvFrame(VENC_CHN VeChn,
                const VENC_RECV_PIC_PARAM_S *pstRecvParam)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    venc_enc_ctx *pEncCtx = NULL;
    venc_chn_vars *pChnVars = NULL;
    venc_vb_ctx *pVbCtx = NULL;
    VENC_ATTR_S *pVencAttr = NULL;
    MMF_CHN_S stBindSrc;
    MMF_CHN_S chn = {.enModId = CVI_ID_VENC, .s32DevId = 0, .s32ChnId = VeChn};
    int isBindMode = FALSE;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;
    pEncCtx = &pChnHandle->encCtx;
    pVbCtx = pChnHandle->pVbCtx;
    pVencAttr = &pChnHandle->pChnAttr->stVencAttr;

    pChnVars->s32RecvPicNum = pstRecvParam->s32RecvPicNum;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }

    s32Ret = cviSetRcParamToDrv(pChnHandle);
    if (s32Ret < 0) {
        CVI_VENC_ERR("cviSetRcParamToDrv\n");
        MUTEX_UNLOCK(&pChnHandle->chnMutex);
        return s32Ret;
    }

    s32Ret = cviSetCuPredToDrv(pChnHandle);
    if (s32Ret < 0) {
        CVI_VENC_ERR("cviSetCuPredToDrv\n");
        return s32Ret;
    }

    if (pEncCtx->base.ioctl) {
        if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H265 ||
            pChnHandle->pChnAttr->stVencAttr.enType == PT_H264) {
            s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_START, (void *)&pChnHandle->VeChn);
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("CVI_H26X_OP_START, %d\n", s32Ret);
                MUTEX_UNLOCK(&pChnHandle->chnMutex);
                return -1;
            }
        } /*else {
            s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_JPEG_OP_START,
                (CVI_VOID *)&pChnHandle->pChnAttr->stVencAttr.bIsoSendFrmEn);
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("CVI_JPEG_OP_START, %d\n", s32Ret);
                MUTEX_UNLOCK(&pChnHandle->chnMutex);
                return -1;
            }
        }*/
    }

    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    pChnVars->chnState = VENC_CHN_STATE_START_ENC;
    handle->chn_status[VeChn] = VENC_CHN_STATE_START_ENC;

    s32Ret = bind_get_src(&chn, &stBindSrc);
    CVI_VENC_DBG("get bind src, ret:%d\n", s32Ret);
    if (s32Ret == CVI_SUCCESS) {
        // struct sched_param param = {
        //     .sched_priority = 95,
        // };
        if (pEncCtx->base.ioctl) {
            if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H265 ||
                pChnHandle->pChnAttr->stVencAttr.enType == PT_H264) {
                isBindMode = TRUE;
                s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_BIND_MODE, (void *)&isBindMode);
                if (s32Ret != CVI_SUCCESS) {
                    CVI_VENC_ERR("CVI_H26X_OP_SET_BIND_MODE, %d\n", s32Ret);
                    return -1;
                }
            }
        }

        pVbCtx->currBindMode = CVI_TRUE;
        pChnHandle->bChnEnable = CVI_TRUE;
        if (!pVbCtx->thread) {
            pVbCtx->thread = kthread_run(venc_event_handler,
                            (CVI_VOID *)pChnHandle,
                            "soph_vc_bh%d", VeChn);

            if (IS_ERR(pVbCtx->thread)) {
                CVI_VENC_ERR(
                    "failed to create venc binde mode thread for chn %d\n",
                    VeChn);
                return CVI_FAILURE;
            }
            // sched_setscheduler(pVbCtx->thread, SCHED_RR, &param);
            SEMA_POST(&pChnVars->sem_send);
        }
    }

    return CVI_SUCCESS;
}

CVI_S32 CVI_VENC_StopRecvFrame(VENC_CHN VeChn)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    venc_vb_ctx *pVbCtx = NULL;
    venc_chn_vars *pChnVars = NULL;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;
    pVbCtx = pChnHandle->pVbCtx;

    if (pVbCtx->currBindMode == CVI_TRUE) {
        SEMA_POST(&pChnVars->sem_release);
        pChnHandle->bChnEnable = CVI_FALSE;
        SEMA_POST(&pVbCtx->vb_jobs.sem);
        kthread_stop(pVbCtx->thread);
        pVbCtx->thread = NULL;
        pVbCtx->currBindMode = CVI_FALSE;
        SEMA_POST(&pChnVars->sem_send);
        CVI_VENC_INFO("venc_event_handler end\n");
    }

    pChnVars->chnState = VENC_CHN_STATE_STOP_ENC;
    handle->chn_status[VeChn] = VENC_CHN_STATE_STOP_ENC;

    return s32Ret;
}

/**
 * @brief Query the current channel status of encoder
 * @param[in] VeChn VENC Channel Number.
 * @param[out] pstStatus Current channel status of encoder
 * @retval 0 Success
 * @retval Non-0 Failure, Please see the error code table
 */
CVI_S32 CVI_VENC_QueryStatus(VENC_CHN VeChn, VENC_CHN_STATUS_S *pstStatus)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    VENC_ATTR_S *pVencAttr = NULL;
    VENC_CHN_STATUS_S *pChnStat = NULL;
    venc_chn_vars *pChnVars = NULL;
    VENC_CHN_ATTR_S *pChnAttr = NULL;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;
    pChnAttr = pChnHandle->pChnAttr;
    pChnStat = &pChnVars->chnStatus;
    pVencAttr = &pChnAttr->stVencAttr;

    memcpy(pstStatus, pChnStat, sizeof(VENC_CHN_STATUS_S));
    pstStatus->u32CurPacks = _cviGetNumPacks(pVencAttr->enType);

    return s32Ret;
}



CVI_S32 CVI_VENC_SendFrameEx(VENC_CHN VeChn,
                 const USER_FRAME_INFO_S *pstUserFrame,
                 CVI_S32 s32MilliSec)
{
    const VIDEO_FRAME_INFO_S *pstFrame = &pstUserFrame->stUserFrame;
    cviUserRcInfo userRcInfo, *puri = &userRcInfo;
    venc_chn_context *pChnHandle = NULL;
    venc_chn_vars *pChnVars = NULL;
    venc_enc_ctx *pEncCtx = NULL;
    CVI_S32 s32Ret = CVI_SUCCESS;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;
    pEncCtx = &pChnHandle->encCtx;

    memcpy(&pChnVars->stUserRcInfo, &pstUserFrame->stUserRcInfo,
           sizeof(USER_RC_INFO_S));

    puri->bQpMapValid = pChnVars->stUserRcInfo.bQpMapValid;
    puri->u64QpMapPhyAddr = pChnVars->stUserRcInfo.u64QpMapPhyAddr;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }
    s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_USER_RC_INFO,
                     (CVI_VOID *)puri);
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("CVI_H26X_OP_SET_USER_RC_INFO, %X\n", s32Ret);
        return s32Ret;
    }

    s32Ret = CVI_VENC_SendFrame(VeChn, pstFrame, s32MilliSec);

    return s32Ret;
}


/**
 * @brief Get encoded bitstream
 * @param[in] VeChn VENC Channel Number.
 * @param[out] pstStream pointer to VIDEO_FRAME_INFO_S.
 * @param[in] S32MilliSec TODO VENC
 * @retval 0 Success
 * @retval Non-0 Failure, please see the error code table
 */
CVI_S32 CVI_VENC_GetStream(VENC_CHN VeChn, VENC_STREAM_S *pstStream,
               CVI_S32 S32MilliSec)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    venc_enc_ctx *pEncCtx = NULL;
    venc_chn_vars *pChnVars = NULL;
    CVI_U64 getStream_e;
    VENC_CHN_ATTR_S *pChnAttr;
    venc_vb_ctx *pVbCtx = NULL;
    VENC_ATTR_S *pVencAttr = NULL;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    pChnVars = pChnHandle->pChnVars;
    pChnAttr = pChnHandle->pChnAttr;
    pVbCtx = pChnHandle->pVbCtx;
    pVencAttr = &pChnAttr->stVencAttr;

    if (pChnHandle->bSbSkipFrm == true) {
        //CVI_VENC_ERR("SbSkipFrm\n");
        return CVI_ERR_VENC_BUSY;
    }

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
    pstStream->pstPack = vmalloc(sizeof(VENC_PACK_S) * _cviGetNumPacks(pChnAttr->stVencAttr.enType));
    if (pstStream->pstPack == NULL)
        return CVI_ERR_VENC_NOMEM;
#endif

    if (!pVencAttr->bIsoSendFrmEn && pVbCtx->currBindMode == CVI_TRUE) {
        CVI_VENC_INFO("wait encode done\n");
        //sem_wait(&pChnVars->sem_send);

        s32Ret = cviVenc_sem_timedwait_Millsecs(&pChnVars->sem_send,
                            S32MilliSec);
        if (s32Ret == CVI_FAILURE) {
            CVI_VENC_ERR("sem wait error\n");
            return CVI_ERR_VENC_MUTEX_ERROR;

        } else if (s32Ret == CVI_ERR_VENC_BUSY) {
            CVI_VENC_INFO("sem wait timeout\n");
            return s32Ret;
        }

        s32Ret = updateStreamPackInBindMode(pChnHandle, pChnVars,
                            pstStream);
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR(
                "updateStreamPackInBindMode fail, s32Ret = 0x%X\n",
                s32Ret);
            return s32Ret;
        }

        cviUpdateChnStatus(pChnHandle);

        return CVI_SUCCESS;
    }

    s32Ret = pEncCtx->base.getStream(pEncCtx, pstStream, S32MilliSec);
    if (s32Ret != CVI_SUCCESS) {
        if ((S32MilliSec >= 0) && (s32Ret == CVI_ERR_VENC_BUSY)) {
            //none block mode: timeout
            CVI_VENC_DBG("Non-blcok mode timeout\n");
        } else {
            //block mode
            if (s32Ret != CVI_ERR_VENC_EMPTY_STREAM_FRAME && s32Ret != CVI_ERR_VENC_GET_STREAM_END)
                CVI_VENC_ERR("getStream, s32Ret = %d\n", s32Ret);
        }

        return s32Ret;
    }

    s32Ret = cviProcessResult(pChnHandle, pstStream);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("(chn %d) cviProcessResult fail\n", VeChn);
        return s32Ret;
    }
    getStream_e = get_current_time();
    pChnVars->totalTime += (getStream_e - pChnVars->u64TimeOfSendFrame);

    cviUpdateChnStatus(pChnHandle);

    return s32Ret;
}

CVI_S32 CVI_VENC_ReleaseStream(VENC_CHN VeChn, VENC_STREAM_S *pstStream)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    venc_enc_ctx *pEncCtx = NULL;
    VENC_CHN_ATTR_S *pChnAttr;
    venc_chn_vars *pChnVars = NULL;
    venc_vb_ctx *pVbCtx = NULL;
    VENC_ATTR_S *pVencAttr;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    pChnVars = pChnHandle->pChnVars;
    pVbCtx = pChnHandle->pVbCtx;
    pChnAttr = pChnHandle->pChnAttr;
    pVencAttr = &pChnAttr->stVencAttr;

    if (!pVencAttr->bIsoSendFrmEn && pVbCtx->currBindMode == CVI_TRUE) {
        CVI_VENC_INFO("[%d]post release\n", pChnVars->frameIdx);
        SEMA_POST(&pChnVars->sem_release);
    } else {
        s32Ret = pEncCtx->base.releaseStream(pEncCtx, pstStream);
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("releaseStream, s32Ret = %d\n", s32Ret);
            return s32Ret;
        }
    }

    pChnVars->frameIdx++;

    if (pChnHandle->jpgFrmSkipCnt) {
        pChnHandle->jpgFrmSkipCnt--;
    }

    return s32Ret;
}


/**
 * @brief Send encoder module parameters
 * @param[in] VENC_PARAM_MOD_S.
 * @retval 0 Success
 * @retval Non-0 Failure, please see the error code table
 */
CVI_S32 CVI_VENC_SetModParam(const VENC_PARAM_MOD_S *pstModParam)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_context *pVencHandle;
    CVI_U32 *pUserDataMaxLen;

    if (pstModParam == NULL) {
        CVI_VENC_ERR("pstModParam NULL !\n");
        return CVI_ERR_VENC_ILLEGAL_PARAM;
    }

    s32Ret = _cviVencInitCtxHandle();
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("CVI_VENC_GetModParam  init failure\n");
        return s32Ret;
    }

    pVencHandle = (venc_context *)handle;
    if (pVencHandle == NULL) {
        CVI_VENC_ERR(
            "CVI_VENC_SetModParam venc_context global handle not create\n");
        return CVI_ERR_VENC_NULL_PTR;

    } else {
        if (pstModParam->enVencModType == MODTYPE_H264E) {
            memcpy(&pVencHandle->ModParam.stH264eModParam,
                   &pstModParam->stH264eModParam,
                   sizeof(VENC_MOD_H264E_S));

            pUserDataMaxLen =
                &(pVencHandle->ModParam.stH264eModParam
                      .u32UserDataMaxLen);
            *pUserDataMaxLen =
                MIN(*pUserDataMaxLen, USERDATA_MAX_LIMIT);
        } else if (pstModParam->enVencModType == MODTYPE_H265E) {
            memcpy(&pVencHandle->ModParam.stH265eModParam,
                   &pstModParam->stH265eModParam,
                   sizeof(VENC_MOD_H265E_S));

            pUserDataMaxLen =
                &(pVencHandle->ModParam.stH265eModParam
                      .u32UserDataMaxLen);
            *pUserDataMaxLen =
                MIN(*pUserDataMaxLen, USERDATA_MAX_LIMIT);
        } else if (pstModParam->enVencModType == MODTYPE_JPEGE) {
            s32Ret = _cvi_venc_check_jpege_format(
                &pstModParam->stJpegeModParam);
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR(
                    "CVI_VENC_SetModParam  JPEG marker order error\n");
                return s32Ret;
            }

            memcpy(&pVencHandle->ModParam.stJpegeModParam,
                   &pstModParam->stJpegeModParam,
                   sizeof(VENC_MOD_JPEGE_S));
            _cvi_venc_config_jpege_format(
                &pVencHandle->ModParam.stJpegeModParam);
        }
    }

    return s32Ret;
}

/**
 * @brief Get encoder module parameters
 * @param[in] VENC_PARAM_MOD_S.
 * @retval 0 Success
 * @retval Non-0 Failure, please see the error code table
 */
CVI_S32 CVI_VENC_GetModParam(VENC_PARAM_MOD_S *pstModParam)
{
    CVI_S32 s32Ret;
    venc_context *pVencHandle;

    if (pstModParam == NULL) {
        CVI_VENC_ERR("pstModParam NULL !\n");
        return CVI_ERR_VENC_ILLEGAL_PARAM;
    }

    s32Ret = _cviVencInitCtxHandle();
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("CVI_VENC_GetModParam  init failure\n");
        return s32Ret;
    }

    pVencHandle = (venc_context *)handle;
    if (pVencHandle == NULL) {
        CVI_VENC_ERR(
            "CVI_VENC_GetModParam venc_context global handle not create\n");
        return CVI_ERR_VENC_NULL_PTR;

    } else {
        if (pstModParam->enVencModType == MODTYPE_H264E) {
            memcpy(&pstModParam->stH264eModParam,
                   &pVencHandle->ModParam.stH264eModParam,
                   sizeof(VENC_MOD_H264E_S));
        } else if (pstModParam->enVencModType == MODTYPE_H265E) {
            memcpy(&pstModParam->stH265eModParam,
                   &pVencHandle->ModParam.stH265eModParam,
                   sizeof(VENC_MOD_H265E_S));
        } else if (pstModParam->enVencModType == MODTYPE_JPEGE) {
            memcpy(&pstModParam->stJpegeModParam,
                   &pVencHandle->ModParam.stJpegeModParam,
                   sizeof(VENC_MOD_JPEGE_S));
        }
    }

    return CVI_SUCCESS;
}

/**
 * @brief encoder module attach vb buffer
 * @param[in] VeChn VENC Channel Number.
 * @param[in] pstPool VENC_CHN_POOL_S.
 * @retval 0 Success
 * @retval Non-0 Failure, please see the error code table
 */
CVI_S32 CVI_VENC_AttachVbPool(VENC_CHN VeChn, const VENC_CHN_POOL_S *pstPool)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    venc_context *p_venctx_handle = handle;
    VB_SOURCE_E eVbSource = VB_SOURCE_COMMON;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = p_venctx_handle->chn_handle[VeChn];

    if (pstPool == NULL) {
        CVI_VENC_ERR("pstPool = NULL\n");
        return CVI_ERR_VENC_ILLEGAL_PARAM;
    }

    if (pChnHandle->pChnAttr->stVencAttr.enType == PT_JPEG ||
        pChnHandle->pChnAttr->stVencAttr.enType == PT_MJPEG) {
        CVI_VENC_ERR("Not support Picture type\n");
        return CVI_ERR_VENC_NOT_SUPPORT;

    } else if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H264)
        eVbSource = p_venctx_handle->ModParam.stH264eModParam
                    .enH264eVBSource;
    else if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H265)
        eVbSource = p_venctx_handle->ModParam.stH265eModParam
                    .enH265eVBSource;

    if (eVbSource != VB_SOURCE_USER) {
        CVI_VENC_ERR("Not support eVbSource:%d\n", eVbSource);
        return CVI_ERR_VENC_NOT_SUPPORT;
    }

    pChnHandle->pChnVars->bHasVbPool = CVI_TRUE;
    pChnHandle->pChnVars->vbpool = *pstPool;

    return CVI_SUCCESS;
}

/**
 * @brief encoder module detach vb buffer
 * @param[in] VeChn VENC Channel Number.
 * @retval 0 Success
 * @retval Non-0 Failure, please see the error code table
 */
CVI_S32 CVI_VENC_DetachVbPool(VENC_CHN VeChn)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    venc_context *p_venctx_handle = handle;
    venc_chn_vars *pChnVars = NULL;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = p_venctx_handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;
    if (pChnVars->bHasVbPool == CVI_FALSE) {
        CVI_VENC_ERR("VeChn= %d vbpool not been attached\n", VeChn);
        return CVI_ERR_VENC_NOT_SUPPORT;

    } else {
        if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H265 ||
            (pChnHandle->pChnAttr->stVencAttr.enType == PT_H264)) {


            #ifdef USE_VB_POOL
            int i = 0;
            for (i = 0; i < (int)pChnVars->FrmNum; i++) {
                VB_BLK blk;

                blk = vb_phys_addr2handle(
                    pChnVars->FrmArray[i].phyAddr);
                if (blk != VB_INVALID_HANDLE)
                    vb_release_block(blk);
            }
            #endif
        } else {
            CVI_VENC_ERR("Not Support Type with bHasVbPool on\n");
            return CVI_ERR_VENC_NOT_SUPPORT;
        }

        pChnVars->bHasVbPool = CVI_FALSE;
    }

    return CVI_SUCCESS;
}

/**
 * @brief Send one frame to encode
 * @param[in] VeChn VENC Channel Number.
 * @param[in] pstFrame pointer to VIDEO_FRAME_INFO_S.
 * @param[in] s32MilliSec TODO VENC
 * @retval 0 Success
 * @retval Non-0 Failure, please see the error code table
 */
CVI_S32 CVI_VENC_SendFrame(VENC_CHN VeChn, const VIDEO_FRAME_INFO_S *pstFrame,
               CVI_S32 s32MilliSec)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    venc_enc_ctx *pEncCtx = NULL;
    venc_chn_vars *pChnVars = NULL;
    VENC_CHN_STATUS_S *pChnStat = NULL;
    int i = 0;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    pChnVars = pChnHandle->pChnVars;
    pChnStat = &pChnVars->chnStatus;

    if (pChnVars->chnState != VENC_CHN_STATE_START_ENC) {
        return CVI_ERR_VENC_INIT;
    }

    if (pChnVars->bSendFirstFrm == false) {
        CVI_BOOL bCbCrInterleave = 0;
        CVI_BOOL bNV21 = 0;

        switch (pstFrame->stVFrame.enPixelFormat) {
        case PIXEL_FORMAT_NV12:
            bCbCrInterleave = true;
            bNV21 = false;
            break;
        case PIXEL_FORMAT_NV16:
        case PIXEL_FORMAT_NV61:
            bCbCrInterleave = true;
            bNV21 = false;
            break;
        case PIXEL_FORMAT_NV21:
            bCbCrInterleave = true;
            bNV21 = true;
            break;
        case PIXEL_FORMAT_YUV_PLANAR_420:
        default:
            bCbCrInterleave = false;
            bNV21 = false;
            break;
        }

        _cviVencSetInPixelFormat(VeChn, bCbCrInterleave, bNV21);

        s32Ret = _cviVencAllocVbBuf(VeChn);
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("_cviVencAllocVbBuf\n");
            return CVI_ERR_VENC_NOBUF;
        }

        if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H264 ||
            pChnHandle->pChnAttr->stVencAttr.enType == PT_H265) {
            _cviVencRegVbBuf(VeChn);
        }

        pChnVars->u32FirstPixelFormat =
            pstFrame->stVFrame.enPixelFormat;
        pChnVars->bSendFirstFrm = true;
    } else {
        if (pChnVars->u32FirstPixelFormat !=
            pstFrame->stVFrame.enPixelFormat) {
            if (pChnHandle->pChnAttr->stVencAttr.enType ==
                    PT_H264 ||
                pChnHandle->pChnAttr->stVencAttr.enType ==
                    PT_H265) {
                CVI_VENC_ERR("Input enPixelFormat change\n");
                return CVI_ERR_VENC_ILLEGAL_PARAM;
            }
        }
    }

    for (i = 0; i < 3; i++) {
        pChnVars->u32Stride[i] = pstFrame->stVFrame.u32Stride[i];
    }

    pChnVars->u32SendFrameCnt++;

#ifdef CLI_DEBUG_SUPPORT
    cviCliDumpSrcYuv(VeChn, pstFrame);
#endif

    s32Ret = cviSetVencChnAttrToProc(VeChn, pstFrame);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("(chn %d) cviSetVencChnAttrToProc fail\n", VeChn);
        return s32Ret;
    }
    pChnVars->u64TimeOfSendFrame = get_current_time();

    if (pEncCtx->base.bVariFpsEn) {
        s32Ret = cviCheckFps(pChnHandle, pstFrame);
        if (s32Ret == CVI_ERR_VENC_STAT_VFPS_CHANGE) {
            pChnVars->bAttrChange = CVI_TRUE;
            s32Ret = cviSetChnAttr(pChnHandle);
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("cviSetChnAttr, %d\n", s32Ret);
                return s32Ret;
            }
        } else if (s32Ret < 0) {
            CVI_VENC_ERR("cviCheckFps, %d\n", s32Ret);
            return s32Ret;
        }

        s32Ret =
            pEncCtx->base.encOnePic(pEncCtx, pstFrame, s32MilliSec);
        if (s32Ret != CVI_SUCCESS) {
            if (s32Ret == CVI_ERR_VENC_BUSY)
                CVI_VENC_WARN("chn:%d encOnePic Error, VPU is busy\n", VeChn);
            else
                CVI_VENC_ERR("chn:%d encOnePic Error, s32Ret = %d\n", VeChn, s32Ret);
            return s32Ret;
        }
    } else {
        if (pChnVars->bAttrChange == CVI_TRUE) {
            s32Ret = cviSetChnAttr(pChnHandle);
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("cviSetChnAttr, %d\n", s32Ret);
                return s32Ret;
            }
        }

        s32Ret = cviCheckFrc(pChnHandle);
        if (s32Ret <= 0) {
            cviUpdateFrcSrc(pChnHandle);
            return CVI_ERR_VENC_FRC_NO_ENC;
        }

        s32Ret =
            pEncCtx->base.encOnePic(pEncCtx, pstFrame, s32MilliSec);
        if (s32Ret != CVI_SUCCESS) {
            if (s32Ret == CVI_ERR_VENC_BUSY)
                CVI_VENC_WARN("encOnePic Error, VPU is busy\n");
            else
                CVI_VENC_ERR("encOnePic chn %d Error, s32Ret = %d\n",
                         VeChn, s32Ret);

            return s32Ret;
        }

        cviUpdateFrcDst(pChnHandle);
        cviUpdateFrcSrc(pChnHandle);
        cviCheckFrcOverflow(pChnHandle);
    }

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }
    pChnStat->u32LeftStreamFrames++;
    if (pChnHandle->pChnAttr->stVencAttr.enType == PT_JPEG ||
            pChnHandle->pChnAttr->stVencAttr.enType == PT_MJPEG) {
        wake_up(&tVencWaitQueue[VeChn]);
    }
    pChnVars->currPTS = pstFrame->stVFrame.u64PTS;
    pEncCtx->base.u64PTS = pChnVars->currPTS;
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    return s32Ret;
}


/**
 * @brief Destroy One VENC Channel.
 * @param[in] VeChn VENC Channel Number.
 * @retval 0 Success
 * @retval Non-0 Failure, please see the error code table
 */
CVI_S32 CVI_VENC_DestroyChn(VENC_CHN VeChn)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    venc_enc_ctx *pEncCtx = NULL;
    venc_chn_vars *pChnVars = NULL;
    venc_vb_ctx *pVbCtx = NULL;
    // MMF_CHN_S chn = { .enModId = CVI_ID_VENC,
    //           .s32DevId = 0,
    //           .s32ChnId = 0};

    if (handle == NULL || handle->chn_handle[VeChn] == NULL) {
        CVI_VENC_INFO("VdChn: %d already destoryed.\n", VeChn);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    pChnVars = pChnHandle->pChnVars;
    pVbCtx = pChnHandle->pVbCtx;

    base_mod_jobs_exit(&vencVbCtx[VeChn].vb_jobs);

    s32Ret = pEncCtx->base.close(pEncCtx);

    SEMA_DESTROY(&pChnVars->sem_send);
    SEMA_DESTROY(&pChnVars->sem_release);

    if (pChnVars->bHasVbPool == CVI_TRUE &&
        (pChnHandle->pChnAttr->stVencAttr.enType == PT_H264 ||
        pChnHandle->pChnAttr->stVencAttr.enType == PT_H265)) {
        //CVI_U32 i = 0;
        VB_SOURCE_E eVbSource;

        if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H264) {
            eVbSource =
                handle->ModParam.stH264eModParam.enH264eVBSource;
        } else if (pChnHandle->pChnAttr->stVencAttr.enType == PT_H265) {
            eVbSource =
                handle->ModParam.stH265eModParam.enH265eVBSource;
        } else {
            eVbSource = VB_SOURCE_COMMON;
        }

        #ifdef USE_VB_POOL
        for (i = 0; i < pChnVars->FrmNum; i++) {
            VB_BLK blk;

            blk = vb_phys_addr2handle(
                pChnVars->FrmArray[i].phyAddr);
            if (blk != VB_INVALID_HANDLE)
                vb_release_block(blk);
        }

        if (eVbSource == VB_SOURCE_PRIVATE) {
            vb_destroy_pool(pChnVars->vbpool.hPicVbPool);
        }
        #endif

        pChnVars->bHasVbPool = CVI_FALSE;
    }

    if (pChnHandle->pChnVars) {
        MEM_FREE(pChnHandle->pChnVars);
        pChnHandle->pChnVars = NULL;
    }

    if (pChnHandle->pChnAttr) {
        MEM_FREE(pChnHandle->pChnAttr);
        pChnHandle->pChnAttr = NULL;
    }

    MUTEX_DESTROY(&pChnHandle->chnMutex);
    MUTEX_DESTROY(&pChnHandle->chnShmMutex);

    if (handle->chn_handle[VeChn]) {
        MEM_FREE(handle->chn_handle[VeChn]);
        handle->chn_handle[VeChn] = NULL;
    }

    handle->chn_status[VeChn] = VENC_CHN_STATE_NONE;

    {
        VENC_CHN i = 0;
        CVI_BOOL bFreeVencHandle = CVI_TRUE;

        for (i = 0; i < VENC_MAX_CHN_NUM; i++) {
            if (handle->chn_handle[i] != NULL) {
                bFreeVencHandle = CVI_FALSE;
                break;
            }
        }
    }

    return s32Ret;
}

/**
 * @brief Resets a VENC channel.
 * @param[in] VeChn VENC Channel Number.
 * @retval 0 Success
 * @retval Non-0 Failure, please see the error code table
 */
CVI_S32 CVI_VENC_ResetChn(VENC_CHN VeChn)
{
    /* from Hisi,
     * You are advised to call HI_MPI_VENC_ResetChn
     * to reset the channel before switching the format of the input image
     * from non-single-component format to single-component format.
     */
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    PAYLOAD_TYPE_E enType;
    venc_chn_vars *pChnVars = NULL;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;

    if (pChnVars->chnState == VENC_CHN_STATE_START_ENC) {
        CVI_VENC_ERR("Chn %d is not stopped, failure\n", VeChn);
        return CVI_ERR_VENC_BUSY;
    }

    enType = pChnHandle->pChnAttr->stVencAttr.enType;
    if (enType == PT_JPEG) {
        venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;
        if (pEncCtx->base.ioctl) {
            s32Ret = pEncCtx->base.ioctl(
                pEncCtx, CVI_JPEG_OP_RESET_CHN, NULL);
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR(
                    "CVI_JPEG_OP_RESET_CHN fail, s32Ret = %d\n",
                    s32Ret);
                return s32Ret;
            }
        }
    }

    // TODO: + Check resolution to decide if release memory / + re-init FW

    return s32Ret;
}

CVI_S32 CVI_VENC_SetJpegParam(VENC_CHN VeChn,
                  const VENC_JPEG_PARAM_S *pstJpegParam)
{
    venc_chn_context *pChnHandle = NULL;
    VENC_JPEG_PARAM_S *pvjp = NULL;
    CVI_S32 s32Ret = CVI_SUCCESS;
    VENC_MJPEG_FIXQP_S *pstMJPEGFixQp;
    VENC_RC_ATTR_S *prcatt;
    venc_chn_vars *pChnVars;
    venc_enc_ctx *pEncCtx;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    pChnVars = pChnHandle->pChnVars;
    pvjp = &pChnVars->stJpegParam;

    if (pEncCtx->base.ioctl) {
        s32Ret = pEncCtx->base.ioctl(
            pEncCtx, CVI_JPEG_OP_SET_MCUPerECS,
            (CVI_VOID *)&pstJpegParam->u32MCUPerECS);
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("CVI_JPEG_OP_SET_MCUPerECS, %d\n", s32Ret);
            return -1;
        }
    }

    if (pstJpegParam->u32Qfactor > Q_TABLE_MAX) {
        CVI_VENC_ERR("u32Qfactor <= 0 || >= 100, s32Ret = %d\n",
                 s32Ret);
        return CVI_ERR_VENC_ILLEGAL_PARAM;
    }

    prcatt = &pChnHandle->pChnAttr->stRcAttr;
    pstMJPEGFixQp = &prcatt->stMjpegFixQp;

    prcatt->enRcMode = VENC_RC_MODE_MJPEGFIXQP;
    pstMJPEGFixQp->u32Qfactor = pstJpegParam->u32Qfactor;

    if (pEncCtx->base.ioctl) {
        s32Ret = pEncCtx->base.ioctl(
            pEncCtx, CVI_JPEG_OP_SET_QUALITY,
            (CVI_VOID *)pstJpegParam);
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("CVI_JPEG_OP_SET_QUALITY, %d\n", s32Ret);
            return -1;
        }
    }

    memcpy(pvjp, pstJpegParam, sizeof(VENC_JPEG_PARAM_S));

    return s32Ret;
}

CVI_S32 CVI_VENC_GetJpegParam(VENC_CHN VeChn, VENC_JPEG_PARAM_S *pstJpegParam)
{
    venc_chn_context *pChnHandle = NULL;
    venc_chn_vars *pChnVars = NULL;
    VENC_JPEG_PARAM_S *pvjp = NULL;
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_enc_ctx *pEncCtx;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    pChnVars = pChnHandle->pChnVars;

    pvjp = &pChnVars->stJpegParam;

     if (pEncCtx->base.ioctl) {
        s32Ret = pEncCtx->base.ioctl(
            pEncCtx, CVI_JPEG_OP_GET_QUALITY,
            (CVI_VOID *)pvjp);
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("CVI_JPEG_OP_SET_QUALITY, %d\n", s32Ret);
            return -1;
        }
    }

    memcpy(pstJpegParam, &pChnVars->stJpegParam, sizeof(VENC_JPEG_PARAM_S));

    return s32Ret;
}

CVI_S32 CVI_VENC_SetMjpegParam(VENC_CHN VeChn,
                  const VENC_MJPEG_PARAM_S *pstMJpegParam)
{
    venc_chn_context *pChnHandle = NULL;
    VENC_MJPEG_PARAM_S *pvmjpe = NULL;
    CVI_S32 s32Ret = CVI_SUCCESS;
    VENC_MJPEG_FIXQP_S *pstMJPEGFixQp;
    VENC_JPEG_PARAM_S stJpegParam = {0};
    VENC_RC_ATTR_S *prcatt;
    venc_chn_vars *pChnVars;
    venc_enc_ctx *pEncCtx;
    CVI_S32 i;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    pChnVars = pChnHandle->pChnVars;
    pvmjpe = &pChnVars->stMjpegParam;

    if (pEncCtx->base.ioctl) {
        s32Ret = pEncCtx->base.ioctl(
            pEncCtx, CVI_JPEG_OP_SET_MCUPerECS,
            (CVI_VOID *)&pstMJpegParam->u32MCUPerECS);
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("CVI_JPEG_OP_SET_MCUPerECS, %d\n", s32Ret);
            return -1;
        }
    }

    prcatt = &pChnHandle->pChnAttr->stRcAttr;

    // only FixQp mod can set quality by user
    if (prcatt->enRcMode == VENC_RC_MODE_MJPEGFIXQP) {
        pstMJPEGFixQp = &prcatt->stMjpegFixQp;

        stJpegParam.u32Qfactor = pstMJPEGFixQp->u32Qfactor;
        stJpegParam.u32MCUPerECS = pstMJpegParam->u32MCUPerECS;
        for (i = 0; i < ARRAY_SIZE(stJpegParam.u8YQt); i++) {
            stJpegParam.u8YQt[i]   = pstMJpegParam->u8YQt[i];
            stJpegParam.u8CbQt[i]  = pstMJpegParam->u8CbQt[i];
            stJpegParam.u8CrQt[i]  = pstMJpegParam->u8CrQt[i];
        }

        if (pEncCtx->base.ioctl) {
            s32Ret = pEncCtx->base.ioctl(
                pEncCtx, CVI_JPEG_OP_SET_QUALITY,
                (CVI_VOID *)&stJpegParam);
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR("CVI_JPEG_OP_SET_QUALITY, %d\n", s32Ret);
                return -1;
            }
        }
    }

    memcpy(pvmjpe, pstMJpegParam, sizeof(VENC_MJPEG_PARAM_S));

    return s32Ret;
}

CVI_S32 CVI_VENC_GetMjpegParam(VENC_CHN VeChn, VENC_MJPEG_PARAM_S *pstMJpegParam)
{
    venc_chn_context *pChnHandle = NULL;
    venc_chn_vars *pChnVars = NULL;
    VENC_MJPEG_PARAM_S *pvmjpe = NULL;
    VENC_JPEG_PARAM_S stJpegParam = {0};
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_enc_ctx *pEncCtx;
    CVI_S32 i;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    pChnVars = pChnHandle->pChnVars;

    pvmjpe = &pChnVars->stMjpegParam;

     if (pEncCtx->base.ioctl) {
         s32Ret = pEncCtx->base.ioctl(
             pEncCtx, CVI_JPEG_OP_GET_QUALITY,
             (CVI_VOID *)&stJpegParam);
         if (s32Ret != CVI_SUCCESS) {
             CVI_VENC_ERR("CVI_JPEG_OP_SET_QUALITY, %d\n", s32Ret);
             return -1;
         }
     }

     for (i = 0; i < ARRAY_SIZE(stJpegParam.u8YQt); i++) {
         pvmjpe->u8YQt[i]   = stJpegParam.u8YQt[i];
         pvmjpe->u8CbQt[i]  = stJpegParam.u8CbQt[i];
         pvmjpe->u8CrQt[i]  = stJpegParam.u8CrQt[i];
     }

    memcpy(pstMJpegParam, &pChnVars->stMjpegParam, sizeof(VENC_MJPEG_PARAM_S));

    return s32Ret;
}

CVI_S32 CVI_VENC_RequestIDR(VENC_CHN VeChn, CVI_BOOL bInstant)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    PAYLOAD_TYPE_E enType;
    venc_enc_ctx *pEncCtx;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    pEncCtx = &pChnHandle->encCtx;

    if (enType == PT_H264 || enType == PT_H265) {
        if (pEncCtx->base.ioctl) {
            if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
                CVI_VENC_ERR("can not lock chnMutex\n");
                return CVI_FAILURE;
            }
            s32Ret =
                pEncCtx->base.ioctl(pEncCtx,
                            CVI_H26X_OP_SET_REQUEST_IDR,
                            (CVI_VOID *)&bInstant);
            MUTEX_UNLOCK(&pChnHandle->chnMutex);
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR(
                    "CVI_H26X_OP_SET_REQUEST_IDR, %d\n",
                    s32Ret);
                return s32Ret;
            }
        }
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_EnableIDR(VENC_CHN VeChn, CVI_BOOL bEnableIDR)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    PAYLOAD_TYPE_E enType;
    venc_enc_ctx *pEncCtx;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;
    pEncCtx = &pChnHandle->encCtx;

    if (enType == PT_H264 || enType == PT_H265) {
        if (pEncCtx->base.ioctl) {
            if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
                CVI_VENC_ERR("can not lock chnMutex\n");
                return CVI_FAILURE;
            }
            s32Ret =
                pEncCtx->base.ioctl(pEncCtx,
                            CVI_H26X_OP_SET_ENABLE_IDR,
                            (CVI_VOID *)&bEnableIDR);
            MUTEX_UNLOCK(&pChnHandle->chnMutex);
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR(
                    "CVI_H26X_OP_SET_REQUEST_IDR, %d\n",
                    s32Ret);
                return s32Ret;
            }
        }
    }

    return s32Ret;
}


CVI_S32 CVI_VENC_SetChnAttr(VENC_CHN VeChn, const VENC_CHN_ATTR_S *pstChnAttr)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    VENC_RC_ATTR_S *pstRcAttr = NULL;
    VENC_ATTR_S *pstVencAttr = NULL;

    if (pstChnAttr == NULL) {
        CVI_VENC_ERR("pstChnAttr == NULL\n");
        return CVI_ERR_VENC_ILLEGAL_PARAM;
    }

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];

    if ((pChnHandle->pChnAttr == NULL) || (pChnHandle->pChnVars == NULL)) {
        CVI_VENC_ERR("pChnAttr or pChnVars is NULL\n");
        return CVI_ERR_VENC_NULL_PTR;
    }

    if (pstChnAttr->stVencAttr.enType !=
        pChnHandle->pChnAttr->stVencAttr.enType) {
        CVI_VENC_ERR("enType is incorrect\n");
        return CVI_ERR_VENC_NOT_SUPPORT;
    }

    pstRcAttr = &(pChnHandle->pChnAttr->stRcAttr);
    pstVencAttr = &(pChnHandle->pChnAttr->stVencAttr);

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }
    memcpy(pstRcAttr, &pstChnAttr->stRcAttr, sizeof(VENC_RC_ATTR_S));
    memcpy(pstVencAttr, &pstChnAttr->stVencAttr, sizeof(VENC_ATTR_S));
    pChnHandle->pChnVars->bAttrChange = CVI_TRUE;
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    return s32Ret;
}

CVI_S32 CVI_VENC_GetChnAttr(VENC_CHN VeChn, VENC_CHN_ATTR_S *pstChnAttr)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle Error, s32Ret = %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }
    memcpy(pstChnAttr, pChnHandle->pChnAttr, sizeof(VENC_CHN_ATTR_S));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    return s32Ret;
}

CVI_S32 CVI_VENC_GetRcParam(VENC_CHN VeChn, VENC_RC_PARAM_S *pstRcParam)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];

    memcpy(pstRcParam, &pChnHandle->rcParam, sizeof(VENC_RC_PARAM_S));

    return s32Ret;
}

CVI_S32 CVI_VENC_SetRcParam(VENC_CHN VeChn, const VENC_RC_PARAM_S *pstRcParam)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    VENC_CHN_ATTR_S *pChnAttr;
    VENC_ATTR_S *pVencAttr;
    VENC_RC_PARAM_S *prcparam;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnAttr = pChnHandle->pChnAttr;
    pVencAttr = &pChnAttr->stVencAttr;
    prcparam = &pChnHandle->rcParam;

    s32Ret = cviCheckRcParam(pChnHandle, pstRcParam);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("cviCheckRcParam\n");
        return s32Ret;
    }

    memcpy(prcparam, pstRcParam, sizeof(VENC_RC_PARAM_S));

    if (pVencAttr->enType == PT_H264 || pVencAttr->enType == PT_H265) {
        prcparam->s32FirstFrameStartQp =
            (prcparam->s32FirstFrameStartQp < 0 ||
             prcparam->s32FirstFrameStartQp > 51) ?
                      ((pVencAttr->enType == PT_H265) ? 63 :
                                    DEF_IQP) :
                      prcparam->s32FirstFrameStartQp;

        if (pstRcParam->s32FirstFrameStartQp !=
            prcparam->s32FirstFrameStartQp) {
            CVI_VENC_DBG(
                "pstRcParam 1stQp = %d, prcparam 1stQp = %d\n",
                pstRcParam->s32FirstFrameStartQp,
                prcparam->s32FirstFrameStartQp);
        }
    }

    s32Ret = cviSetRcParamToDrv(pChnHandle);
    if (s32Ret < 0) {
        CVI_VENC_ERR("cviSetRcParamToDrv\n");
        return s32Ret;
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_GetRefParam(VENC_CHN VeChn, VENC_REF_PARAM_S *pstRefParam)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];

    memcpy(pstRefParam, &pChnHandle->refParam, sizeof(VENC_REF_PARAM_S));

    return s32Ret;
}

CVI_S32 CVI_VENC_SetRefParam(VENC_CHN VeChn,
                 const VENC_REF_PARAM_S *pstRefParam)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    venc_enc_ctx *pEncCtx;
    PAYLOAD_TYPE_E enType;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];

    memcpy(&pChnHandle->refParam, pstRefParam, sizeof(VENC_REF_PARAM_S));

    pEncCtx = &pChnHandle->encCtx;

    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (pEncCtx->base.ioctl && (enType == PT_H264 || enType == PT_H265)) {
        unsigned int tempLayer = 1;

        if (pstRefParam->u32Base == 1 && pstRefParam->u32Enhance == 1 &&
            pstRefParam->bEnablePred == CVI_TRUE)
            tempLayer = 2;
        else if (pstRefParam->u32Base == 2 &&
             pstRefParam->u32Enhance == 1 &&
             pstRefParam->bEnablePred == CVI_TRUE)
            tempLayer = 3;

        s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_REF_PARAM,
                         (CVI_VOID *)&tempLayer);
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("CVI_H26X_OP_SET_REF_PARAM, %d\n", s32Ret);
            return -1;
        }
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_SetCuPrediction(VENC_CHN VeChn,
                 const VENC_CU_PREDICTION_S *pstCuPrediction)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    VENC_CU_PREDICTION_S *pcup;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pcup = &pChnHandle->pChnVars->cuPrediction;

    s32Ret = (pstCuPrediction->u32IntraCost <= CVI_H26X_INTRACOST_MAX) ?
        CVI_SUCCESS : CVI_ERR_VENC_CU_PREDICTION;
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("error, %s = %d\n",
            "u32IntraCost",
            pstCuPrediction->u32IntraCost);
        return s32Ret;
    }

    memcpy(pcup, pstCuPrediction, sizeof(VENC_CU_PREDICTION_S));

    s32Ret = cviSetCuPredToDrv(pChnHandle);
    if (s32Ret < 0) {
        CVI_VENC_ERR("cviSetCuPredToDrv\n");
        return s32Ret;
    }

    return s32Ret;
}


CVI_S32 CVI_VENC_GetCuPrediction(VENC_CHN VeChn,
                 VENC_CU_PREDICTION_S *pstCuPrediction)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    VENC_CU_PREDICTION_S *pcup;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pcup = &pChnHandle->pChnVars->cuPrediction;

    memcpy(pstCuPrediction, pcup, sizeof(VENC_CU_PREDICTION_S));

    return s32Ret;
}

CVI_S32 CVI_VENC_GetRoiAttr(VENC_CHN VeChn, CVI_U32 u32Index,
                VENC_ROI_ATTR_S *pstRoiAttr)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    venc_enc_ctx *pEncCtx;
    PAYLOAD_TYPE_E enType;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];

    pEncCtx = &pChnHandle->encCtx;

    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (pEncCtx->base.ioctl && (enType == PT_H264 || enType == PT_H265)) {
        cviRoiParam RoiParam;

        memset(&RoiParam, 0x0, sizeof(cviRoiParam));
        RoiParam.roi_index = u32Index;
        s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_GET_ROI_PARAM,
                         (CVI_VOID *)&RoiParam);
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("CVI_H26X_OP_SET_ROI_PARAM, %d\n", s32Ret);
            return -1;
        }
        pstRoiAttr->u32Index = RoiParam.roi_index;
        pstRoiAttr->bEnable = RoiParam.roi_enable_flag;
        pstRoiAttr->bAbsQp = RoiParam.is_absolute_qp;
        pstRoiAttr->s32Qp = RoiParam.roi_qp;
        pstRoiAttr->stRect.s32X = RoiParam.roi_rect_x;
        pstRoiAttr->stRect.s32Y = RoiParam.roi_rect_y;
        pstRoiAttr->stRect.u32Width = RoiParam.roi_rect_width;
        pstRoiAttr->stRect.u32Height = RoiParam.roi_rect_height;
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_SetFrameLostStrategy(VENC_CHN VeChn,
                      const VENC_FRAMELOST_S *pstFrmLostParam)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    PAYLOAD_TYPE_E enType;
    venc_enc_ctx *pEncCtx;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];

    enType = pChnHandle->pChnAttr->stVencAttr.enType;
    pEncCtx = &pChnHandle->encCtx;

    if (enType == PT_H264 || enType == PT_H265) {
        if (pEncCtx->base.ioctl) {
            cviFrameLost frameLostSetting;

            if (pstFrmLostParam->bFrmLostOpen) {
                // If frame lost is on, check parameters
                if (pstFrmLostParam->enFrmLostMode !=
                    FRMLOST_PSKIP) {
                    CVI_VENC_ERR(
                        "frame lost mode = %d, unsupport\n",
                        (int)(pstFrmLostParam
                                  ->enFrmLostMode));
                    return CVI_ERR_VENC_NOT_SUPPORT;
                }

                frameLostSetting.frameLostMode =
                    pstFrmLostParam->enFrmLostMode;
                frameLostSetting.u32EncFrmGaps =
                    pstFrmLostParam->u32EncFrmGaps;
                frameLostSetting.bFrameLostOpen =
                    pstFrmLostParam->bFrmLostOpen;
                frameLostSetting.u32FrmLostBpsThr =
                    pstFrmLostParam->u32FrmLostBpsThr;
            } else {
                // set gap and threshold back to 0
                frameLostSetting.frameLostMode =
                    pstFrmLostParam->enFrmLostMode;
                frameLostSetting.u32EncFrmGaps = 0;
                frameLostSetting.bFrameLostOpen = 0;
                frameLostSetting.u32FrmLostBpsThr = 0;
            }
            CVI_VENC_DBG(
                "bFrameLostOpen = %d, frameLostMode = %d, u32EncFrmGaps = %d, u32FrmLostBpsThr = %d\n",
                frameLostSetting.bFrameLostOpen,
                frameLostSetting.frameLostMode,
                frameLostSetting.u32EncFrmGaps,
                frameLostSetting.u32FrmLostBpsThr);
            memcpy(&pChnHandle->pChnVars->frameLost,
                   pstFrmLostParam, sizeof(VENC_FRAMELOST_S));

            if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
                CVI_VENC_ERR("can not lock chnMutex\n");
                return CVI_FAILURE;
            }
            s32Ret = pEncCtx->base.ioctl(
                pEncCtx, CVI_H26X_OP_SET_FRAME_LOST_STRATEGY,
                (CVI_VOID *)(&frameLostSetting));
            MUTEX_UNLOCK(&pChnHandle->chnMutex);
            if (s32Ret != CVI_SUCCESS) {
                CVI_VENC_ERR(
                    "CVI_H26X_OP_SET_FRAME_LOST_STRATEGY, %d\n",
                    s32Ret);
                return s32Ret;
            }
        }
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_SetRoiAttr(VENC_CHN VeChn, const VENC_ROI_ATTR_S *pstRoiAttr)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;
    venc_enc_ctx *pEncCtx;
    PAYLOAD_TYPE_E enType;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];

    pEncCtx = &pChnHandle->encCtx;

    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (pEncCtx->base.ioctl && (enType == PT_H264 || enType == PT_H265)) {
        cviRoiParam RoiParam;

        if (pChnHandle->pChnAttr->stRcAttr.enRcMode ==
            VENC_RC_MODE_H265FIXQP) {
            CVI_VENC_ERR(
                "[H265]ROI does not support FIX QP mode.\n");
            return -1;
        }
        memset(&RoiParam, 0x0, sizeof(cviRoiParam));
        RoiParam.roi_index = pstRoiAttr->u32Index;
        RoiParam.roi_enable_flag = pstRoiAttr->bEnable;
        RoiParam.is_absolute_qp = pstRoiAttr->bAbsQp;
        RoiParam.roi_qp = pstRoiAttr->s32Qp;
        RoiParam.roi_rect_x = pstRoiAttr->stRect.s32X;
        RoiParam.roi_rect_y = pstRoiAttr->stRect.s32Y;
        RoiParam.roi_rect_width = pstRoiAttr->stRect.u32Width;
        RoiParam.roi_rect_height = pstRoiAttr->stRect.u32Height;

        s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_ROI_PARAM,
                         (CVI_VOID *)&RoiParam);
        if (s32Ret != CVI_SUCCESS) {
            CVI_VENC_ERR("CVI_H26X_OP_SET_ROI_PARAM, %d\n", s32Ret);
            return -1;
        }

        if (MUTEX_LOCK(&pChnHandle->chnShmMutex) != 0) {
            CVI_VENC_ERR("can not lock chnShmMutex\n");
            return CVI_FAILURE;
        }
        memcpy(&pChnHandle->pChnVars->stRoiAttr[pstRoiAttr->u32Index],
               pstRoiAttr, sizeof(VENC_ROI_ATTR_S));
        MUTEX_UNLOCK(&pChnHandle->chnShmMutex);
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_GetFrameLostStrategy(VENC_CHN VeChn,
                      VENC_FRAMELOST_S *pstFrmLostParam)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle = NULL;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    if (pChnHandle->pChnVars) {
        memcpy(pstFrmLostParam, &pChnHandle->pChnVars->frameLost,
               sizeof(VENC_FRAMELOST_S));
    } else {
        s32Ret = CVI_ERR_VENC_NOT_CONFIG;
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_SetChnParam(VENC_CHN VeChn,
                 const VENC_CHN_PARAM_S *pstChnParam)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    venc_chn_context *pChnHandle;
    venc_chn_vars *pChnVars;
    RECT_S *prect;
    venc_enc_ctx_base *pvecb;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;
    prect = &pChnVars->stChnParam.stCropCfg.stRect;
    pvecb = &pChnHandle->encCtx.base;

    memcpy(&pChnVars->stChnParam, pstChnParam, sizeof(VENC_CHN_PARAM_S));

    if ((prect->s32X & 0xF) || (prect->s32Y & 0xF)) {
        prect->s32X &= (~0xF);
        prect->s32Y &= (~0xF);
    }

    pvecb->x = prect->s32X;
    pvecb->y = prect->s32Y;

    return s32Ret;
}

CVI_S32 CVI_VENC_GetChnParam(VENC_CHN VeChn, VENC_CHN_PARAM_S *pstChnParam)
{
    CVI_S32 s32Ret = CVI_SUCCESS;
    VENC_CHN_PARAM_S *pvcp;
    venc_chn_context *pChnHandle;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pvcp = &pChnHandle->pChnVars->stChnParam;

    memcpy(pstChnParam, pvcp, sizeof(VENC_CHN_PARAM_S));

    return s32Ret;
}

CVI_S32 CVI_VENC_InsertUserData(VENC_CHN VeChn, CVI_U8 *pu8Data, CVI_U32 u32Len)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    PAYLOAD_TYPE_E enType;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pu8Data == NULL || u32Len == 0) {
        s32Ret = CVI_FAILURE_ILLEGAL_PARAM;
        CVI_VENC_ERR("no user data\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (!pEncCtx->base.ioctl) {
        CVI_VENC_ERR("base.ioctl is NULL\n");
        return CVI_ERR_VENC_NULL_PTR;
    }

    if (enType == PT_JPEG || enType == PT_MJPEG) {
        s32Ret = cviVencJpegInsertUserData(pChnHandle, pu8Data, u32Len);
    } else if (enType == PT_H264 || enType == PT_H265) {
        s32Ret = cviVencH26xInsertUserData(pChnHandle, pu8Data, u32Len);
    }

    if (s32Ret != CVI_SUCCESS)
        CVI_VENC_ERR("failed to insert user data %d", s32Ret);
    return s32Ret;
}


CVI_S32 CVI_VENC_SetH264Entropy(VENC_CHN VeChn,
                const VENC_H264_ENTROPY_S *pstH264EntropyEnc)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    VENC_ATTR_S *pVencAttr;
    venc_enc_ctx *pEncCtx;
    VENC_H264_ENTROPY_S h264Entropy;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264EntropyEnc == NULL) {
        CVI_VENC_ERR("no user data\n");
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pVencAttr = &pChnHandle->pChnAttr->stVencAttr;
    pEncCtx = &pChnHandle->encCtx;

    if (pVencAttr->enType != PT_H264) {
        CVI_VENC_ERR("not h264 encode channel\n");
        return CVI_ERR_VENC_NOT_SUPPORT;
    }

    if (pVencAttr->u32Profile == H264E_PROFILE_BASELINE) {
        if (pstH264EntropyEnc->u32EntropyEncModeI !=
                H264E_ENTROPY_CAVLC ||
            pstH264EntropyEnc->u32EntropyEncModeP !=
                H264E_ENTROPY_CAVLC) {
            CVI_VENC_ERR(
                "h264 Baseline Profile only supports CAVLC\n");
            return CVI_ERR_VENC_NOT_SUPPORT;
        }
    }

    if (!pEncCtx->base.ioctl) {
        CVI_VENC_ERR("base.ioctl is NULL\n");
        return CVI_ERR_VENC_NULL_PTR;
    }

    memcpy(&h264Entropy, pstH264EntropyEnc, sizeof(VENC_H264_ENTROPY_S));
    s32Ret = cviVencH264SetEntropy(pChnHandle, &h264Entropy);

    if (s32Ret == CVI_SUCCESS) {
        memcpy(&pChnHandle->h264Entropy, &h264Entropy,
               sizeof(VENC_H264_ENTROPY_S));
    } else {
        CVI_VENC_ERR("failed to set h264 entropy %d", s32Ret);
        s32Ret = CVI_ERR_VENC_H264_ENTROPY;
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_GetH264Entropy(VENC_CHN VeChn,
                VENC_H264_ENTROPY_S *pstH264EntropyEnc)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    PAYLOAD_TYPE_E enType;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264EntropyEnc == NULL) {
        s32Ret = CVI_FAILURE_ILLEGAL_PARAM;
        CVI_VENC_ERR("no h264 entropy param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264) {
        CVI_VENC_ERR("not h264 encode channel\n");
        return CVI_ERR_VENC_NOT_SUPPORT;
    }

    memcpy(pstH264EntropyEnc, &pChnHandle->h264Entropy,
           sizeof(VENC_H264_ENTROPY_S));
    return CVI_SUCCESS;
}

CVI_S32 CVI_VENC_SetH264Trans(VENC_CHN VeChn,
                  const VENC_H264_TRANS_S *pstH264Trans)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    PAYLOAD_TYPE_E enType;
    VENC_H264_TRANS_S h264Trans;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264Trans == NULL) {
        CVI_VENC_ERR("no h264 trans param\n");
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264) {
        CVI_VENC_ERR("not h264 encode channel\n");
        return CVI_ERR_VENC_NOT_SUPPORT;
    }

    if (!pEncCtx->base.ioctl) {
        CVI_VENC_ERR("base.ioctl is NULL\n");
        return CVI_ERR_VENC_NULL_PTR;
    }

    memcpy(&h264Trans, pstH264Trans, sizeof(VENC_H264_TRANS_S));
    s32Ret = cviVencH264SetTrans(pChnHandle, &h264Trans);

    if (s32Ret == CVI_SUCCESS) {
        memcpy(&pChnHandle->h264Trans, &h264Trans,
               sizeof(VENC_H264_TRANS_S));
    } else {
        CVI_VENC_ERR("failed to set h264 trans %d", s32Ret);
        s32Ret = CVI_ERR_VENC_H264_TRANS;
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_GetH264Trans(VENC_CHN VeChn, VENC_H264_TRANS_S *pstH264Trans)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    PAYLOAD_TYPE_E enType;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264Trans == NULL) {
        s32Ret = CVI_FAILURE_ILLEGAL_PARAM;
        CVI_VENC_ERR("no h264 trans param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264) {
        CVI_VENC_ERR("not h264 encode channel\n");
        return CVI_ERR_VENC_NOT_SUPPORT;
    }

    memcpy(pstH264Trans, &pChnHandle->h264Trans, sizeof(VENC_H264_TRANS_S));
    return CVI_SUCCESS;
}

CVI_S32 CVI_VENC_SetH265PredUnit(VENC_CHN VeChn, const VENC_H265_PU_S *pstPredUnit)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    PAYLOAD_TYPE_E enType;
    VENC_H265_PU_S h265PredUnit;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstPredUnit == NULL) {
        CVI_VENC_ERR("no h265 PredUnit\n");
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        CVI_VENC_ERR("not h265 encode channel\n");
        return CVI_ERR_VENC_NOT_SUPPORT;
    }

    if (!pEncCtx->base.ioctl) {
        CVI_VENC_ERR("base.ioctl is NULL\n");
        return CVI_ERR_VENC_NULL_PTR;
    }

    memcpy(&h265PredUnit, pstPredUnit, sizeof(VENC_H265_PU_S));
    s32Ret = cviVencH265SetPredUnit(pChnHandle, &h265PredUnit);

    if (s32Ret == CVI_SUCCESS) {
        memcpy(&pChnHandle->h265PredUnit, &h265PredUnit,
               sizeof(VENC_H265_PU_S));
    } else {
        CVI_VENC_ERR("failed to set h265 PredUnit %d", s32Ret);
        s32Ret = CVI_ERR_VENC_H265_PRED_UNIT;
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_GetH265PredUnit(VENC_CHN VeChn, VENC_H265_PU_S *pstPredUnit)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    PAYLOAD_TYPE_E enType;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstPredUnit == NULL) {
        s32Ret = CVI_FAILURE_ILLEGAL_PARAM;
        CVI_VENC_ERR("no h265 PredUnit\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        CVI_VENC_ERR("not h265 encode channel\n");
        return CVI_ERR_VENC_NOT_SUPPORT;
    }

    s32Ret = cviVencH265GetPredUnit(pChnHandle, &pChnHandle->h265PredUnit);

    if (s32Ret == CVI_SUCCESS) {
        memcpy(pstPredUnit, &pChnHandle->h265PredUnit, sizeof(VENC_H265_PU_S));
    } else {
        CVI_VENC_ERR("failed to get h265 PredUnit %d", s32Ret);
        s32Ret = CVI_ERR_VENC_H265_PRED_UNIT;
    }

    return CVI_SUCCESS;
}

CVI_S32 CVI_VENC_SetH265Trans(VENC_CHN VeChn,
                  const VENC_H265_TRANS_S *pstH265Trans)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    PAYLOAD_TYPE_E enType;
    VENC_H265_TRANS_S h265Trans;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH265Trans == NULL) {
        CVI_VENC_ERR("no h265 trans param\n");
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        CVI_VENC_ERR("not h265 encode channel\n");
        return CVI_ERR_VENC_NOT_SUPPORT;
    }

    if (!pEncCtx->base.ioctl) {
        CVI_VENC_ERR("base.ioctl is NULL\n");
        return CVI_ERR_VENC_NULL_PTR;
    }

    memcpy(&h265Trans, pstH265Trans, sizeof(VENC_H265_TRANS_S));
    s32Ret = cviVencH265SetTrans(pChnHandle, &h265Trans);

    if (s32Ret == CVI_SUCCESS) {
        memcpy(&pChnHandle->h265Trans, &h265Trans,
               sizeof(VENC_H265_TRANS_S));
    } else {
        CVI_VENC_ERR("failed to set h265 trans %d", s32Ret);
        s32Ret = CVI_ERR_VENC_H265_TRANS;
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_GetH265Trans(VENC_CHN VeChn, VENC_H265_TRANS_S *pstH265Trans)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    PAYLOAD_TYPE_E enType;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH265Trans == NULL) {
        s32Ret = CVI_FAILURE_ILLEGAL_PARAM;
        CVI_VENC_ERR("no h265 trans param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        CVI_VENC_ERR("not h265 encode channel\n");
        return CVI_ERR_VENC_NOT_SUPPORT;
    }

    memcpy(pstH265Trans, &pChnHandle->h265Trans, sizeof(VENC_H265_TRANS_S));
    return CVI_SUCCESS;
}

CVI_S32
CVI_VENC_SetSuperFrameStrategy(VENC_CHN VeChn,
                   const VENC_SUPERFRAME_CFG_S *pstSuperFrmParam)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_chn_vars *pChnVars = NULL;
    venc_enc_ctx *pEncCtx;
    cviSuperFrame super, *pSuper = &super;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstSuperFrmParam == NULL) {
        CVI_VENC_ERR("pstSuperFrmParam = NULL\n");
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;
    pEncCtx = &pChnHandle->encCtx;
    pSuper->enSuperFrmMode =
        (pstSuperFrmParam->enSuperFrmMode == SUPERFRM_NONE) ?
                  CVI_SUPERFRM_NONE :
                  CVI_SUPERFRM_REENCODE_IDR;
    pSuper->u32SuperIFrmBitsThr = pstSuperFrmParam->u32SuperIFrmBitsThr;
    pSuper->u32SuperPFrmBitsThr = pstSuperFrmParam->u32SuperPFrmBitsThr;

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }
    s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_SUPER_FRAME,
                     (CVI_VOID *)pSuper);
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret == CVI_SUCCESS) {
        memcpy(&pChnVars->stSuperFrmParam, pstSuperFrmParam,
               sizeof(VENC_SUPERFRAME_CFG_S));
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_GetSuperFrameStrategy(VENC_CHN VeChn,
                       VENC_SUPERFRAME_CFG_S *pstSuperFrmParam)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_chn_vars *pChnVars = NULL;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstSuperFrmParam == NULL) {
        s32Ret = CVI_FAILURE_ILLEGAL_PARAM;
        CVI_VENC_ERR("pstSuperFrmParam = NULL\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;

    memcpy(pstSuperFrmParam, &pChnVars->stSuperFrmParam,
           sizeof(VENC_SUPERFRAME_CFG_S));
    return CVI_SUCCESS;
}

CVI_S32 CVI_VENC_CalcFrameParam(VENC_CHN VeChn,
                VENC_FRAME_PARAM_S *pstFrameParam)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    cviFrameParam frmp, *pfp = &frmp;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstFrameParam == NULL) {
        CVI_VENC_ERR("pstFrameParam is NULL\n");
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;

    if (!pEncCtx->base.ioctl) {
        CVI_VENC_ERR("base.ioctl is NULL\n");
        return CVI_FAILURE;
    }

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }
    s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_CALC_FRAME_PARAM,
                     (CVI_VOID *)(pfp));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("failed to calculate FrameParam %d", s32Ret);
        s32Ret = CVI_ERR_VENC_FRAME_PARAM;
    }

    pstFrameParam->u32FrameQp = pfp->u32FrameQp;
    pstFrameParam->u32FrameBits = pfp->u32FrameBits;
    return s32Ret;
}

CVI_S32 CVI_VENC_SetFrameParam(VENC_CHN VeChn,
                   const VENC_FRAME_PARAM_S *pstFrameParam)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    venc_chn_vars *pChnVars;
    cviFrameParam frmp, *pfp = &frmp;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstFrameParam == NULL) {
        CVI_VENC_ERR("pstFrameParam is NULL\n");
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;
    pEncCtx = &pChnHandle->encCtx;

    if (!pEncCtx->base.ioctl) {
        CVI_VENC_ERR("base.ioctl is NULL\n");
        return CVI_FAILURE;
    }

    pfp->u32FrameQp = pstFrameParam->u32FrameQp;
    pfp->u32FrameBits = pstFrameParam->u32FrameBits;

    s32Ret = (pfp->u32FrameQp <= CVI_H26X_FRAME_QP_MAX) ?
        CVI_SUCCESS : CVI_ERR_VENC_ILLEGAL_PARAM;
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("error, %s = %d\n",
            "u32FrameQp",
            pfp->u32FrameQp);
        return s32Ret;
    }
    s32Ret = ((pfp->u32FrameBits >= CVI_H26X_FRAME_BITS_MIN) &&
        (pfp->u32FrameBits <= CVI_H26X_FRAME_BITS_MAX)) ?
        CVI_SUCCESS : CVI_ERR_VENC_ILLEGAL_PARAM;
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("error, %s = %d\n",
            "u32FrameBits", pfp->u32FrameBits);
        return s32Ret;
    }

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }
    s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_FRAME_PARAM,
                     (CVI_VOID *)(pfp));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret == CVI_SUCCESS) {
        memcpy(&pChnVars->frameParam, pstFrameParam,
               sizeof(VENC_FRAME_PARAM_S));
    } else {
        CVI_VENC_ERR("failed to set FrameParam %d", s32Ret);
        s32Ret = CVI_ERR_VENC_FRAME_PARAM;
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_GetFrameParam(VENC_CHN VeChn,
                   VENC_FRAME_PARAM_S *pstFrameParam)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_chn_vars *pChnVars;
    VENC_FRAME_PARAM_S *pfp;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstFrameParam == NULL) {
        s32Ret = CVI_FAILURE_ILLEGAL_PARAM;
        CVI_VENC_ERR("no param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pChnVars = pChnHandle->pChnVars;
    pfp = &pChnVars->frameParam;

    memcpy(pstFrameParam, pfp, sizeof(VENC_FRAME_PARAM_S));

    return s32Ret;
}


CVI_S32 CVI_VENC_SetH264Vui(VENC_CHN VeChn, const VENC_H264_VUI_S *pstH264Vui)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    PAYLOAD_TYPE_E enType;
    VENC_H264_VUI_S h264Vui;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264Vui == NULL) {
        CVI_VENC_ERR("no h264 vui param\n");
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264) {
        CVI_VENC_ERR("not h264 encode channel\n");
        return CVI_FAILURE;
    }

    if (!pEncCtx->base.ioctl) {
        CVI_VENC_ERR("base.ioctl is NULL\n");
        return CVI_FAILURE;
    }

    s32Ret = _cviCheckH264VuiParam(pstH264Vui);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("invalid h264 vui param, %d\n", s32Ret);
        return s32Ret;
    }

    memcpy(&h264Vui, pstH264Vui, sizeof(VENC_H264_VUI_S));
    s32Ret = _cviSetH264Vui(pChnHandle, &h264Vui);

    if (s32Ret == CVI_SUCCESS) {
        memcpy(&pChnHandle->h264Vui, &h264Vui, sizeof(VENC_H264_VUI_S));
    } else {
        CVI_VENC_ERR("failed to set h264 vui %d", s32Ret);
        s32Ret = CVI_ERR_VENC_H264_VUI;
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_SetH265Vui(VENC_CHN VeChn, const VENC_H265_VUI_S *pstH265Vui)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    PAYLOAD_TYPE_E enType;
    VENC_H265_VUI_S h265Vui;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH265Vui == NULL) {
        CVI_VENC_ERR("no h265 vui param\n");
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        CVI_VENC_ERR("not h265 encode channel\n");
        return CVI_FAILURE;
    }

    if (!pEncCtx->base.ioctl) {
        CVI_VENC_ERR("base.ioctl is NULL\n");
        return CVI_FAILURE;
    }

    s32Ret = _cviCheckH265VuiParam(pstH265Vui);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("invalid h265 vui param, %d\n", s32Ret);
        return s32Ret;
    }

    memcpy(&h265Vui, pstH265Vui, sizeof(VENC_H265_VUI_S));
    s32Ret = _cviSetH265Vui(pChnHandle, &h265Vui);

    if (s32Ret == CVI_SUCCESS) {
        memcpy(&pChnHandle->h265Vui, &h265Vui, sizeof(VENC_H265_VUI_S));
    } else {
        CVI_VENC_ERR("failed to set h265 vui %d", s32Ret);
        s32Ret = CVI_ERR_VENC_H265_VUI;
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_GetH264Vui(VENC_CHN VeChn, VENC_H264_VUI_S *pstH264Vui)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    PAYLOAD_TYPE_E enType;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264Vui == NULL) {
        s32Ret = CVI_FAILURE_ILLEGAL_PARAM;
        CVI_VENC_ERR("no h264 vui param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264) {
        CVI_VENC_ERR("not h264 encode channel\n");
        return CVI_FAILURE;
    }

    memcpy(pstH264Vui, &pChnHandle->h264Vui, sizeof(VENC_H264_VUI_S));
    return CVI_SUCCESS;
}

CVI_S32 CVI_VENC_GetH265Vui(VENC_CHN VeChn, VENC_H265_VUI_S *pstH265Vui)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    PAYLOAD_TYPE_E enType;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH265Vui == NULL) {
        s32Ret = CVI_FAILURE_ILLEGAL_PARAM;
        CVI_VENC_ERR("no h265 vui param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        CVI_VENC_ERR("not h265 encode channel\n");
        return CVI_FAILURE;
    }

    memcpy(pstH265Vui, &pChnHandle->h265Vui, sizeof(VENC_H265_VUI_S));
    return CVI_SUCCESS;
}

CVI_S32 CVI_VENC_SetH264SliceSplit(VENC_CHN VeChn, const VENC_H264_SLICE_SPLIT_S *pstSliceSplit)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    PAYLOAD_TYPE_E enType;
    VENC_H264_SLICE_SPLIT_S h264Split;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstSliceSplit == NULL) {
        CVI_VENC_ERR("no h264 split param\n");
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    if (pstSliceSplit->bSplitEnable == 1
            && pstSliceSplit->u32MbLineNum == 0) {
        CVI_VENC_ERR("invalid h264 split param\n");
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264) {
        CVI_VENC_ERR("not h264 encode channel\n");
        return CVI_FAILURE;
    }

    if (!pEncCtx->base.ioctl) {
        CVI_VENC_ERR("base.ioctl is NULL\n");
        return CVI_FAILURE;
    }

    memcpy(&h264Split, pstSliceSplit, sizeof(VENC_H264_SLICE_SPLIT_S));
    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }
    s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_H264_SPLIT,
                     (CVI_VOID *)pstSliceSplit);
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret == CVI_SUCCESS) {
        memcpy(&pChnHandle->h264Split, &h264Split, sizeof(VENC_H264_SLICE_SPLIT_S));
    } else {
        CVI_VENC_ERR("failed to set h264 split %d", s32Ret);
        s32Ret = CVI_ERR_VENC_H264_SPLIT;
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_GetH264SliceSplit(VENC_CHN VeChn, VENC_H264_SLICE_SPLIT_S *pstH264Split)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    PAYLOAD_TYPE_E enType;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264Split == NULL) {
        s32Ret = CVI_FAILURE_ILLEGAL_PARAM;
        CVI_VENC_ERR("no h264 split param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264) {
        CVI_VENC_ERR("not h264 encode channel\n");
        return CVI_FAILURE;
    }

    memcpy(pstH264Split, &pChnHandle->h264Split, sizeof(VENC_H264_SLICE_SPLIT_S));
    return CVI_SUCCESS;
}

CVI_S32 CVI_VENC_SetH265SliceSplit(VENC_CHN VeChn, const VENC_H265_SLICE_SPLIT_S *pstSliceSplit)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    PAYLOAD_TYPE_E enType;
    VENC_H265_SLICE_SPLIT_S h265Split;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstSliceSplit == NULL) {
        CVI_VENC_ERR("no h265 split param\n");
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    if (pstSliceSplit->bSplitEnable == 1
            && pstSliceSplit->u32LcuLineNum == 0) {
        CVI_VENC_ERR("invalid h265 split param\n");
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        CVI_VENC_ERR("not h265 encode channel\n");
        return CVI_FAILURE;
    }

    if (!pEncCtx->base.ioctl) {
        CVI_VENC_ERR("base.ioctl is NULL\n");
        return CVI_FAILURE;
    }

    memcpy(&h265Split, pstSliceSplit, sizeof(VENC_H265_SLICE_SPLIT_S));
    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }
    s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_H265_SPLIT,
                     (CVI_VOID *)pstSliceSplit);
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret == CVI_SUCCESS) {
        memcpy(&pChnHandle->h265Split, &h265Split, sizeof(VENC_H265_SLICE_SPLIT_S));
    } else {
        CVI_VENC_ERR("failed to set h265 split %d", s32Ret);
        s32Ret = CVI_ERR_VENC_H265_SPLIT;
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_GetH265SliceSplit(VENC_CHN VeChn, VENC_H265_SLICE_SPLIT_S *pstH265Split)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    PAYLOAD_TYPE_E enType;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH265Split == NULL) {
        s32Ret = CVI_FAILURE_ILLEGAL_PARAM;
        CVI_VENC_ERR("no h265 split param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        CVI_VENC_ERR("not h265 encode channel\n");
        return CVI_FAILURE;
    }

    memcpy(pstH265Split, &pChnHandle->h265Split, sizeof(VENC_H265_SLICE_SPLIT_S));
    return CVI_SUCCESS;
}

CVI_S32 CVI_VENC_SetH264Dblk(VENC_CHN VeChn, const VENC_H264_DBLK_S *pstH264Dblk)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    PAYLOAD_TYPE_E enType;
    VENC_H264_DBLK_S h264Dblk;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264Dblk == NULL) {
        s32Ret = CVI_FAILURE_ILLEGAL_PARAM;
        CVI_VENC_ERR("pstH264Dblk is NULL\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264) {
        CVI_VENC_ERR("not h264 encode channel\n");
        return CVI_FAILURE;
    }

    if (!pEncCtx->base.ioctl) {
        CVI_VENC_ERR("base.ioctl is NULL\n");
        return CVI_FAILURE;
    }

    memcpy(&h264Dblk, pstH264Dblk, sizeof(VENC_H264_DBLK_S));
    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }

    s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_H264_DBLK, (CVI_VOID *)(pstH264Dblk));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);
    if (s32Ret == CVI_SUCCESS) {
        memcpy(&pChnHandle->h264Dblk, pstH264Dblk, sizeof(VENC_H264_DBLK_S));
    } else {
        CVI_VENC_ERR("failed to set h264 dblk %d", s32Ret);
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_GetH264Dblk(VENC_CHN VeChn, VENC_H264_DBLK_S *pstH264Dblk)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264Dblk == NULL) {
        s32Ret = CVI_FAILURE_ILLEGAL_PARAM;
        CVI_VENC_ERR("pstH264Dblk is NULL\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];

    memcpy(pstH264Dblk, &pChnHandle->h264Dblk, sizeof(VENC_H264_DBLK_S));
    return CVI_SUCCESS;
}

CVI_S32 CVI_VENC_SetH265Dblk(VENC_CHN VeChn, const VENC_H265_DBLK_S *pstH265Dblk)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    PAYLOAD_TYPE_E enType;
    VENC_H265_DBLK_S h265Dblk;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH265Dblk == NULL) {
        s32Ret = CVI_FAILURE_ILLEGAL_PARAM;
        CVI_VENC_ERR("pstH265Dblk is NULL\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        CVI_VENC_ERR("not h265 encode channel\n");
        return CVI_FAILURE;
    }

    if (!pEncCtx->base.ioctl) {
        CVI_VENC_ERR("base.ioctl is NULL\n");
        return CVI_FAILURE;
    }

    memcpy(&h265Dblk, pstH265Dblk, sizeof(VENC_H265_DBLK_S));

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }

    s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_H265_DBLK, (CVI_VOID *)(pstH265Dblk));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);
    if (s32Ret == CVI_SUCCESS) {
        memcpy(&pChnHandle->h264Dblk, pstH265Dblk, sizeof(VENC_H265_DBLK_S));
    } else {
        CVI_VENC_ERR("failed to set h265 dblk %d", s32Ret);
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_GetH265Dblk(VENC_CHN VeChn, VENC_H265_DBLK_S *pstH265Dblk)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH265Dblk == NULL) {
        s32Ret = CVI_FAILURE_ILLEGAL_PARAM;
        CVI_VENC_ERR("pstH265Dblk is NULL\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];

    memcpy(pstH265Dblk, &pChnHandle->h265Dblk, sizeof(VENC_H265_DBLK_S));
    return CVI_SUCCESS;
}

CVI_S32 CVI_VENC_SetH264IntraPred(VENC_CHN VeChn, const VENC_H264_INTRA_PRED_S *pstH264IntraPred)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    PAYLOAD_TYPE_E enType;
    VENC_H264_INTRA_PRED_S h264IntraPred;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264IntraPred == NULL) {
        CVI_VENC_ERR("no h264 intra pred param\n");
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    if (pstH264IntraPred->constrained_intra_pred_flag != 0 &&
            pstH264IntraPred->constrained_intra_pred_flag != 1) {
        CVI_VENC_ERR("intra pred param invalid:%u\n"
                    , pstH264IntraPred->constrained_intra_pred_flag);
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264) {
        CVI_VENC_ERR("not h264 encode channel\n");
        return CVI_FAILURE;
    }

    if (!pEncCtx->base.ioctl) {
        CVI_VENC_ERR("base.ioctl is NULL\n");
        return CVI_FAILURE;
    }

    memcpy(&h264IntraPred, pstH264IntraPred, sizeof(VENC_H264_INTRA_PRED_S));
    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }
    s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_H264_INTRA_PRED,
                     (CVI_VOID *)pstH264IntraPred);
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret == CVI_SUCCESS) {
        memcpy(&pChnHandle->h264IntraPred, &h264IntraPred, sizeof(VENC_H264_INTRA_PRED_S));
    } else {
        CVI_VENC_ERR("failed to set intra pred %d", s32Ret);
        s32Ret = CVI_ERR_VENC_H264_INTRA_PRED;
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_GetH264IntraPred(VENC_CHN VeChn, VENC_H264_INTRA_PRED_S *pstH264IntraPred)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    PAYLOAD_TYPE_E enType;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH264IntraPred == NULL) {
        s32Ret = CVI_FAILURE_ILLEGAL_PARAM;
        CVI_VENC_ERR("no h264 intra pred param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264) {
        CVI_VENC_ERR("not h264 encode channel\n");
        return CVI_FAILURE;
    }

    memcpy(pstH264IntraPred, &pChnHandle->h264IntraPred, sizeof(VENC_H264_INTRA_PRED_S));
    return CVI_SUCCESS;
}

CVI_S32 CVI_VENC_SetCustomMap(VENC_CHN VeChn, const VENC_CUSTOM_MAP_S *pstVeCustomMap)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    PAYLOAD_TYPE_E enType;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstVeCustomMap == NULL) {
        s32Ret = CVI_FAILURE_ILLEGAL_PARAM;
        CVI_VENC_ERR("no custom map param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264 && enType != PT_H265) {
        CVI_VENC_ERR("custom map only support h264/h265\n");
        return CVI_FAILURE;
    }

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }
    s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_CUSTOM_MAP,
                     (CVI_VOID *)pstVeCustomMap);
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    return CVI_SUCCESS;
}

CVI_S32 CVI_VENC_GetIntialInfo(VENC_CHN VeChn, VENC_INITIAL_INFO_S *pstVencInitialInfo)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    PAYLOAD_TYPE_E enType;
    cviVencIntialInfo stEncIntialInfo;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstVencInitialInfo == NULL) {
        s32Ret = CVI_FAILURE_ILLEGAL_PARAM;
        CVI_VENC_ERR("no enc intial param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H264 && enType != PT_H265) {
        CVI_VENC_ERR("get enc intinial info only support h264/h265\n");
        return CVI_FAILURE;
    }

    memset(&stEncIntialInfo, 0x0, sizeof(cviVencIntialInfo));
    s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_GET_INITIAL_INFO,
                        (CVI_VOID *)&stEncIntialInfo);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("CVI_H26X_OP_GET_ROI_PARAM, %d\n", s32Ret);
        return -1;
    }

    memcpy(pstVencInitialInfo, &stEncIntialInfo, sizeof(cviVencIntialInfo));
    return s32Ret;
}

CVI_S32 CVI_VENC_SetH265Sao(VENC_CHN VeChn, const VENC_H265_SAO_S *pstH265Sao)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    PAYLOAD_TYPE_E enType;
    VENC_H265_SAO_S h265Sao;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH265Sao == NULL) {
        CVI_VENC_ERR("no h265 sao param\n");
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        CVI_VENC_ERR("not h265 encode channel\n");
        return CVI_FAILURE;
    }

    if (!pEncCtx->base.ioctl) {
        CVI_VENC_ERR("base.ioctl is NULL\n");
        return CVI_FAILURE;
    }

    memcpy(&h265Sao, pstH265Sao, sizeof(VENC_H265_SAO_S));

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }
    s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_SET_H265_SAO,
                  (CVI_VOID *)(pstH265Sao));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret == CVI_SUCCESS) {
        memcpy(&pChnHandle->h265Sao, &h265Sao, sizeof(VENC_H265_SAO_S));
    } else {
        CVI_VENC_ERR("failed to set h265 sao %d", s32Ret);
        s32Ret = CVI_ERR_VENC_H265_SAO;
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_GetH265Sao(VENC_CHN VeChn, VENC_H265_SAO_S *pstH265Sao)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    PAYLOAD_TYPE_E enType;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstH265Sao == NULL) {
        s32Ret = CVI_FAILURE_ILLEGAL_PARAM;
        CVI_VENC_ERR("no h265 sao param\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        CVI_VENC_ERR("not h265 encode channel\n");
        return CVI_FAILURE;
    }

    memcpy(pstH265Sao, &pChnHandle->h265Sao, sizeof(VENC_H265_SAO_S));
    return CVI_SUCCESS;
}

CVI_S32 CVI_VENC_GetEncodeHeader(VENC_CHN VeChn, VENC_ENCODE_HEADER_S *pstEncodeHeader)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    PAYLOAD_TYPE_E enType;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstEncodeHeader == NULL) {
        CVI_VENC_ERR("invalid input param\n");
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265 && enType != PT_H264) {
        CVI_VENC_ERR("not h26x encode channel\n");
        return CVI_FAILURE;
    }

    if (!pEncCtx->base.ioctl) {
        CVI_VENC_ERR("base.ioctl is NULL\n");
        return CVI_FAILURE;
    }

    if (MUTEX_LOCK(&pChnHandle->chnMutex) != 0) {
        CVI_VENC_ERR("can not lock chnMutex\n");
        return CVI_FAILURE;
    }

    s32Ret = pEncCtx->base.ioctl(pEncCtx, CVI_H26X_OP_GET_ENCODE_HEADER,
                  (CVI_VOID *)(pstEncodeHeader));
    MUTEX_UNLOCK(&pChnHandle->chnMutex);

    if (s32Ret == CVI_SUCCESS) {
        // memcpy(&pChnHandle->h265Sao, &h265Sao, sizeof(VENC_H265_SAO_S));
    } else {
        CVI_VENC_ERR("failed to get encode header %d", s32Ret);
        // s32Ret = CVI_ERR_VENC_H265_SAO;
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_SetSearchWindow(VENC_CHN VeChn, const VENC_SEARCH_WINDOW_S *pstVencSearchWindow)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    PAYLOAD_TYPE_E enType;
    VENC_SEARCH_WINDOW_S stVencSearchWindow;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstVencSearchWindow == NULL) {
        CVI_VENC_ERR("no search window\n");
        return CVI_FAILURE_ILLEGAL_PARAM;
    }

    pChnHandle = handle->chn_handle[VeChn];
    pEncCtx = &pChnHandle->encCtx;
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265) {
        CVI_VENC_ERR("not h265 encode channel\n");
        return CVI_ERR_VENC_NOT_SUPPORT;
    }

    if (!pEncCtx->base.ioctl) {
        CVI_VENC_ERR("base.ioctl is NULL\n");
        return CVI_ERR_VENC_NULL_PTR;
    }

    memcpy(&stVencSearchWindow, pstVencSearchWindow, sizeof(VENC_SEARCH_WINDOW_S));
    s32Ret = cviVencSetSearchWindow(pChnHandle, &stVencSearchWindow);

    if (s32Ret == CVI_SUCCESS) {
        memcpy(&pChnHandle->stSearchWindow, &stVencSearchWindow,
               sizeof(VENC_SEARCH_WINDOW_S));
    } else {
        CVI_VENC_ERR("failed to set search window %d", s32Ret);
        s32Ret = CVI_ERR_VENC_SEARCH_WINDOW;
    }

    return s32Ret;
}

CVI_S32 CVI_VENC_GetSearchWindow(VENC_CHN VeChn, VENC_SEARCH_WINDOW_S *pstVencSearchWindow)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    venc_chn_context *pChnHandle;
    PAYLOAD_TYPE_E enType;

    s32Ret = check_chn_handle(VeChn);
    if (s32Ret != CVI_SUCCESS) {
        CVI_VENC_ERR("check_chn_handle, %d\n", s32Ret);
        return s32Ret;
    }

    if (pstVencSearchWindow == NULL) {
        s32Ret = CVI_FAILURE_ILLEGAL_PARAM;
        CVI_VENC_ERR("no search window\n");
        return s32Ret;
    }

    pChnHandle = handle->chn_handle[VeChn];
    enType = pChnHandle->pChnAttr->stVencAttr.enType;

    if (enType != PT_H265 ) {
        CVI_VENC_ERR("not h265 encode channel\n");
        return CVI_ERR_VENC_NOT_SUPPORT;
    }

    s32Ret = cviVencGetSearchWindow(pChnHandle, &pChnHandle->stSearchWindow);

    if (s32Ret == CVI_SUCCESS) {
        memcpy(pstVencSearchWindow, &pChnHandle->stSearchWindow, sizeof(VENC_SEARCH_WINDOW_S));
    } else {
        CVI_VENC_ERR("failed to get search window %d", s32Ret);
        s32Ret = CVI_ERR_VENC_SEARCH_WINDOW;
    }

    return s32Ret;
}
