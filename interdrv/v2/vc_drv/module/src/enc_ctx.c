#include "venc.h"
#include "enc_ctx.h"
#include "module_common.h"
#include "cvi_venc.h"
#include "main_helper.h"

#define JPEG_STREAM_CNT (2)

static CVI_VOID setSrcInfo(CVIFRAMEBUF *psi, venc_enc_ctx *pEncCtx,
               const VIDEO_FRAME_INFO_S *pstFrame)
{
    venc_enc_ctx_base *pvecb = &pEncCtx->base;
    const VIDEO_FRAME_S *pstVFrame;

    pstVFrame = &pstFrame->stVFrame;

    psi->strideY = pstVFrame->u32Stride[0];
    psi->strideC = pstVFrame->u32Stride[1];
    psi->vbY.phys_addr =
        pstVFrame->u64PhyAddr[0] + psi->strideY * pvecb->y + pvecb->x;

    switch (pstVFrame->enPixelFormat) {
    case PIXEL_FORMAT_YUV_PLANAR_422:
        psi->format = CVI_FORMAT_422;
        psi->packedFormat = CVI_PACKED_FORMAT_NONE;
        psi->chromaInterleave = CVI_CBCR_SEPARATED;
        psi->vbCb.phys_addr = pstVFrame->u64PhyAddr[1] +
                      psi->strideC * pvecb->y + pvecb->x / 2;
        psi->vbCr.phys_addr = pstVFrame->u64PhyAddr[2] +
                      psi->strideC * pvecb->y + pvecb->x / 2;
        break;
    case PIXEL_FORMAT_NV16:
    case PIXEL_FORMAT_NV61:
        psi->format = CVI_FORMAT_422;
        psi->packedFormat = CVI_PACKED_FORMAT_NONE;
        psi->chromaInterleave = CVI_CBCR_INTERLEAVE;
        psi->vbCb.phys_addr = pstVFrame->u64PhyAddr[1] +
                      psi->strideC * pvecb->y + pvecb->x / 2;
        psi->vbCr.phys_addr = pstVFrame->u64PhyAddr[2] +
                      psi->strideC * pvecb->y + pvecb->x / 2;
        break;
    case PIXEL_FORMAT_YUYV:
        psi->format = CVI_FORMAT_422;
        psi->packedFormat = CVI_PACKED_FORMAT_422_YUYV;
        psi->chromaInterleave = CVI_CBCR_INTERLEAVE;
        psi->vbCb.phys_addr = 0;
        psi->vbCr.phys_addr = 0;
        break;
    case PIXEL_FORMAT_UYVY:
        psi->format = CVI_FORMAT_422;
        psi->packedFormat = CVI_PACKED_FORMAT_422_UYVY;
        psi->chromaInterleave = CVI_CBCR_INTERLEAVE;
        psi->vbCb.phys_addr = 0;
        psi->vbCr.phys_addr = 0;
        break;
    case PIXEL_FORMAT_YVYU:
        psi->format = CVI_FORMAT_422;
        psi->packedFormat = CVI_PACKED_FORMAT_422_YVYU;
        psi->chromaInterleave = CVI_CBCR_INTERLEAVE;
        psi->vbCb.phys_addr = 0;
        psi->vbCr.phys_addr = 0;
        break;
    case PIXEL_FORMAT_VYUY:
        psi->format = CVI_FORMAT_422;
        psi->packedFormat = CVI_PACKED_FORMAT_422_VYUY;
        psi->chromaInterleave = CVI_CBCR_INTERLEAVE;
        psi->vbCb.phys_addr = 0;
        psi->vbCr.phys_addr = 0;
        break;
    case PIXEL_FORMAT_NV12:
    case PIXEL_FORMAT_NV21:
        psi->format = CVI_FORMAT_420;
        psi->packedFormat = CVI_PACKED_FORMAT_NONE;
        psi->chromaInterleave =
            (pstVFrame->enPixelFormat == PIXEL_FORMAT_NV12) ?
                      CVI_CBCR_INTERLEAVE :
                      CVI_CRCB_INTERLEAVE;
        psi->vbCb.phys_addr = pstVFrame->u64PhyAddr[1] +
                      psi->strideC * pvecb->y / 2 + pvecb->x;
        psi->vbCr.phys_addr = 0;
        break;
    case PIXEL_FORMAT_YUV_PLANAR_444:
        psi->format = CVI_FORMAT_444;
        psi->packedFormat = CVI_PACKED_FORMAT_NONE;
        psi->chromaInterleave = CVI_CBCR_SEPARATED;
        psi->vbCb.phys_addr = pstVFrame->u64PhyAddr[1] +
                      psi->strideC * pvecb->y + pvecb->x;
        psi->vbCr.phys_addr = pstVFrame->u64PhyAddr[2] +
                      psi->strideC * pvecb->y + pvecb->x;
        break;
    case PIXEL_FORMAT_YUV_400:
        psi->format = CVI_FORMAT_400;
        psi->packedFormat = CVI_PACKED_FORMAT_NONE;
        psi->chromaInterleave = CVI_CBCR_SEPARATED;
        psi->vbCb.phys_addr = 0;
        psi->vbCr.phys_addr = 0;
        break;
    case PIXEL_FORMAT_YUV_PLANAR_420:
    default:
        psi->format = CVI_FORMAT_420;
        psi->packedFormat = CVI_PACKED_FORMAT_NONE;
        psi->chromaInterleave = CVI_CBCR_SEPARATED;
        psi->vbCb.phys_addr = pstVFrame->u64PhyAddr[1] +
                      psi->strideC * pvecb->y / 2 +
                      pvecb->x / 2;
        psi->vbCr.phys_addr = pstVFrame->u64PhyAddr[2] +
                      psi->strideC * pvecb->y / 2 +
                      pvecb->x / 2;
        break;
    }

    psi->width = pstVFrame->u32Width;
    psi->height = pstVFrame->u32Height;

    psi->vbY.virt_addr = (void *)pstVFrame->pu8VirAddr[0] +
                 (psi->vbY.phys_addr - pstVFrame->u64PhyAddr[0]);
    psi->vbCb.virt_addr = (void *)pstVFrame->pu8VirAddr[1] +
                  (psi->vbCb.phys_addr - pstVFrame->u64PhyAddr[1]);
    psi->vbCr.virt_addr = (void *)pstVFrame->pu8VirAddr[2] +
                  (psi->vbCr.phys_addr - pstVFrame->u64PhyAddr[2]);

}



static CVI_S32 jpege_init(CVI_VOID)
{
    CVI_S32 status = CVI_SUCCESS;

    status = CVIJpgInit();
    if (status != CVI_SUCCESS) {
        CVIJpgUninit();
        CVI_VENC_ERR("CVIJpgUninit\n");
        return CVI_FAILURE;
    }

    return status;
}

static CVI_S32 jpege_close(CVI_VOID *ctx)
{
    CVI_S32 status = CVI_SUCCESS;
    CVIJpgHandle *pHandle;

    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;

    pHandle = pEncCtx->ext.jpeg.handle;
    if (pHandle != NULL) {
        CVIJpgClose(pHandle);
        pEncCtx->ext.jpeg.handle = NULL;
        CVIJpgUninit();
    }

    return status;
}


static CVI_S32 jpege_open(CVI_VOID *handle, CVI_VOID *pchnctx)
{
    venc_context *pHandle = (venc_context *)handle;
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    CVI_S32 status = CVI_SUCCESS;
    CVIJpgConfig config;
    CVIJpgHandle jpghandle = NULL;
    VENC_RC_ATTR_S *prcatt = NULL;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;

    memset(&config, 0, sizeof(CVIJpgConfig));

    // JPEG marker order
    memcpy(config.u.enc.jpgMarkerOrder,
           pHandle->ModParam.stJpegeModParam.JpegMarkerOrder,
           CVI_JPG_MARKER_ORDER_BUF_SIZE);

    config.type = CVIJPGCOD_ENC;
    config.u.enc.picWidth = pEncCtx->base.width;
    config.u.enc.picHeight = pEncCtx->base.height;
    config.u.enc.chromaInterleave = CVI_CBCR_SEPARATED; // CbCr Separated
    config.u.enc.mirDir = pChnAttr->stVencAttr.enMirrorDirextion;
    config.u.enc.src_type = JPEG_MEM_EXTERNAL;
    config.u.enc.rotAngle = pChnAttr->stVencAttr.enRotation;
    config.u.enc.singleEsBuffer =
        pHandle->ModParam.stJpegeModParam.bSingleEsBuf;
    if (config.u.enc.singleEsBuffer)
        config.u.enc.bitstreamBufSize =
            pHandle->ModParam.stJpegeModParam.u32SingleEsBufSize;
    else
        config.u.enc.bitstreamBufSize = pChnAttr->stVencAttr.u32BufSize;

    if(!config.u.enc.bitstreamBufSize)
        config.u.enc.bitstreamBufSize = CVI_JPG_DEFAULT_BUFSIZE;

    switch (pChnAttr->stVencAttr.enPixelFormat) {
    case PIXEL_FORMAT_YUV_PLANAR_422:
        config.u.enc.sourceFormat = CVI_FORMAT_422;
        config.u.enc.packedFormat = CVI_PACKED_FORMAT_NONE;
        break;
    case PIXEL_FORMAT_YUYV:
        config.u.enc.sourceFormat = CVI_FORMAT_422;
        config.u.enc.packedFormat = CVI_PACKED_FORMAT_422_YUYV;
        config.u.enc.chromaInterleave = CVI_CBCR_INTERLEAVE;
        break;
    case PIXEL_FORMAT_UYVY:
        config.u.enc.sourceFormat = CVI_FORMAT_422;
        config.u.enc.packedFormat = CVI_PACKED_FORMAT_422_UYVY;
        config.u.enc.chromaInterleave = CVI_CBCR_INTERLEAVE;
        break;
    case PIXEL_FORMAT_YVYU:
        config.u.enc.sourceFormat = CVI_FORMAT_422;
        config.u.enc.packedFormat = CVI_PACKED_FORMAT_422_YVYU;
        config.u.enc.chromaInterleave = CVI_CBCR_INTERLEAVE;
        break;
    case PIXEL_FORMAT_VYUY:
        config.u.enc.sourceFormat = CVI_FORMAT_422;
        config.u.enc.packedFormat = CVI_PACKED_FORMAT_422_VYUY;
        config.u.enc.chromaInterleave = CVI_CBCR_INTERLEAVE;
        break;
    case PIXEL_FORMAT_NV16:
        config.u.enc.sourceFormat = CVI_FORMAT_422;
        config.u.enc.packedFormat = CVI_PACKED_FORMAT_NONE;
        config.u.enc.chromaInterleave = CVI_CBCR_INTERLEAVE;
        break;
    case PIXEL_FORMAT_NV61:
        config.u.enc.sourceFormat = CVI_FORMAT_422;
        config.u.enc.packedFormat = CVI_PACKED_FORMAT_NONE;
        config.u.enc.chromaInterleave = CVI_CRCB_INTERLEAVE;
        break;
    case PIXEL_FORMAT_NV12:
        config.u.enc.sourceFormat = CVI_FORMAT_420;
        config.u.enc.packedFormat = CVI_PACKED_FORMAT_NONE;
        config.u.enc.chromaInterleave = CVI_CBCR_INTERLEAVE;
        break;
    case PIXEL_FORMAT_NV21:
        config.u.enc.sourceFormat = CVI_FORMAT_420;
        config.u.enc.packedFormat = CVI_PACKED_FORMAT_NONE;
        config.u.enc.chromaInterleave = CVI_CRCB_INTERLEAVE;
        break;
    case PIXEL_FORMAT_YUV_400:
        config.u.enc.sourceFormat = CVI_FORMAT_400;
        config.u.enc.packedFormat = CVI_PACKED_FORMAT_NONE;
        break;
    case PIXEL_FORMAT_YUV_PLANAR_444:
        config.u.enc.sourceFormat = CVI_FORMAT_444;
        config.u.enc.packedFormat = CVI_PACKED_FORMAT_NONE;
        break;
    case PIXEL_FORMAT_YUV_PLANAR_420:
        config.u.enc.sourceFormat = CVI_FORMAT_420;
        config.u.enc.packedFormat = CVI_PACKED_FORMAT_NONE;
        break;
    default:
        CVI_VENC_ERR("jpeg not support fmt:%d\n",pChnAttr->stVencAttr.enPixelFormat);
        return CVI_ERR_VENC_NOT_SUPPORT;
    }

    CVI_VENC_WARN("fmt:%d sfmt:%d pfmt:%d\n", pChnAttr->stVencAttr.enPixelFormat,
        config.u.enc.sourceFormat, config.u.enc.packedFormat);

    if ((config.u.enc.bitstreamBufSize & 0x3FF) != 0) {
        CVI_VENC_WARN("%s bitstreamBufSize (0x%x) must align to 1024\n",
                 __func__, config.u.enc.bitstreamBufSize);
        config.u.enc.bitstreamBufSize -= config.u.enc.bitstreamBufSize&0x3FF;
        //return CVI_ERR_VENC_NOT_SUPPORT;
    }
    prcatt = &pChnAttr->stRcAttr;

    if (prcatt->enRcMode == VENC_RC_MODE_MJPEGFIXQP) {
        VENC_MJPEG_FIXQP_S *pstMJPEGFixQp = &prcatt->stMjpegFixQp;

        config.u.enc.quality = pstMJPEGFixQp->u32Qfactor;
    } else {
        VENC_MJPEG_CBR_S *pstMJPEGCbr = &prcatt->stMjpegCbr;
        int frameRateDiv, frameRateRes;

        config.u.enc.bitrate = pstMJPEGCbr->u32BitRate;
        frameRateDiv = (pstMJPEGCbr->fr32DstFrameRate >> 16);
        frameRateRes = pstMJPEGCbr->fr32DstFrameRate & 0xFFFF;

        if (frameRateDiv == 0) {
            config.u.enc.framerate = frameRateRes;
        } else {
            config.u.enc.framerate =
                ((frameRateDiv - 1) << 16) + frameRateRes;
        }
    }

    config.s32ChnNum = pChnHandle->VeChn;
    jpghandle = CVIJpgOpen(config);
    if (jpghandle == NULL) {
        CVI_VENC_ERR("%s CVIJpgOpen failed !\n", __func__);
        jpege_close(pEncCtx);
        return CVI_ERR_VENC_NULL_PTR;
    }
    pEncCtx->ext.jpeg.handle = jpghandle;

    return status;
}

static CVI_S32 jpege_enc_one_pic(CVI_VOID *ctx,
                 const VIDEO_FRAME_INFO_S *pstFrame,
                 CVI_S32 s32MIlliSec)
{
    CVIJpgHandle *pHandle;
    CVIFRAMEBUF srcInfo, *psi = &srcInfo;
    CVI_S32 status = CVI_SUCCESS;
    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;

    pHandle = pEncCtx->ext.jpeg.handle;

    setSrcInfo(psi, pEncCtx, pstFrame);

    status = CVIJpgSendFrameData(pHandle, (void *)&srcInfo,
                     JPEG_MEM_EXTERNAL, s32MIlliSec);

    if (status == ENC_TIMEOUT) {
        //jpeg_enc_one_pic TimeOut..dont close
        //otherwise parallel / multiple jpg encode will failure
        return CVI_ERR_VENC_BUSY;
    }

    if (status != CVI_SUCCESS) {
        CVI_VENC_ERR("Failed to CVIJpgSendFrameData, ret = %x\n",
                 status);
        jpege_close(pEncCtx);
        return CVI_ERR_VENC_BUSY;
    }

    return status;
}

static CVI_S32 jpege_get_stream(CVI_VOID *ctx, VENC_STREAM_S *pstStream,
                CVI_S32 s32MilliSec)
{
    SOPHGO_S_USER_BUF sopBuf[2] = { 0 };
    CVI_S32 status = CVI_SUCCESS;
    CVIJpgHandle *pHandle;
    VENC_PACK_S *ppack;
    int i;
    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;

    UNREFERENCED_PARAM(s32MilliSec);

    pHandle = pEncCtx->ext.jpeg.handle;

    status = CVIJpgGetFrameData(
        pHandle, (void *)sopBuf, sizeof(SOPHGO_S_USER_BUF),
        (unsigned long int *)&pEncCtx->base.u64EncHwTime);
#if 0
    if (status != CVI_SUCCESS) {
        CVI_VENC_ERR("Failed to CVIJpgGetFrameData, ret = %x\n",
                 status);
        jpege_close(pEncCtx);
        return CVI_ERR_VENC_BUSY;
    }
#else
    if (status != CVI_SUCCESS) {
        CVI_VENC_ERR("Failed to CVIJpgGetFrameData, ret = %x\n", status);
        return CVI_ERR_VENC_BUSY;
    }
#endif

    pstStream->u32PackCount = JPEG_STREAM_CNT;
    for(i = 0; i < pstStream->u32PackCount; i++) {
        ppack = &pstStream->pstPack[i];

        memset(ppack, 0, sizeof(VENC_PACK_S));

        ppack->pu8Addr = (CVI_U8 *)sopBuf[i].virt_addr;
        ppack->u64PhyAddr = sopBuf[i].phys_addr;
        ppack->u32Len = sopBuf[i].size;
        ppack->u64PTS = pEncCtx->base.u64PTS;
    }

    return status;
}

static CVI_S32 jpege_release_stream(CVI_VOID *ctx, VENC_STREAM_S *pstStream)
{
    CVI_S32 status = CVI_SUCCESS;
    CVIJpgHandle *pHandle;
    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;

    UNREFERENCED_PARAM(pstStream);

    pHandle = pEncCtx->ext.jpeg.handle;
    status = CVIJpgReleaseFrameData(pHandle);

    if (status != CVI_SUCCESS) {
        CVI_VENC_ERR("Failed to CVIJpgReleaseFrameData, ret = %x\n",
                 status);
        jpege_close(pEncCtx);
        return CVI_ERR_VENC_BUSY;
    }

    return CVI_SUCCESS;
}

static CVI_S32 jpege_ioctl(CVI_VOID *ctx, CVI_S32 op, CVI_VOID *arg)
{
    CVI_S32 status = CVI_SUCCESS;
    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;
    CVI_S32 currOp;

    currOp = (op & CVI_JPEG_OP_MASK) >> CVI_JPEG_OP_SHIFT;
    if (currOp == 0) {
        CVI_VENC_WARN("op = 0x%X, currOp = 0x%X\n", op, currOp);
        return 0;
    }

    status = cviJpegIoctl(pEncCtx->ext.jpeg.handle, currOp, arg);
    if (status != CVI_SUCCESS) {
        CVI_VENC_ERR("cviJpgIoctl, currOp = 0x%X, status = %d\n",
                 currOp, status);
        return status;
    }

    return status;
}

static CVI_S32 vidEnc_init(CVI_VOID)
{
    CVI_S32 status = CVI_SUCCESS;
    cviVencInit();
    return status;
}

static CVI_S32 vidEnc_open(CVI_VOID *handle, CVI_VOID *pchnctx)
{
    venc_context *pHandle = (venc_context *)handle;
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    CVI_S32 status = CVI_SUCCESS;
    venc_enc_ctx *pEncCtx = &pChnHandle->encCtx;
    VENC_ATTR_S *pVencAttr = &pChnAttr->stVencAttr;
    VENC_GOP_EX_ATTR_S *pVencGopExAttr = &pChnAttr->stGopExAttr;
    cviInitEncConfig initEncCfg, *pInitEncCfg;

    pInitEncCfg = &initEncCfg;
    memset(pInitEncCfg, 0, sizeof(cviInitEncConfig));

    pInitEncCfg->codec =
        (pVencAttr->enType == PT_H265) ? CODEC_H265 : CODEC_H264;
    pInitEncCfg->width = pEncCtx->base.width;
    pInitEncCfg->height = pEncCtx->base.height;
    pInitEncCfg->u32Profile = pEncCtx->base.u32Profile;
    pInitEncCfg->rcMode = pEncCtx->base.rcMode;
    pInitEncCfg->bitrate = 0;
    if (pInitEncCfg->codec == CODEC_H264) {
        pInitEncCfg->userDataMaxLength =
            pHandle->ModParam.stH264eModParam.u32UserDataMaxLen;
        pInitEncCfg->singleLumaBuf =
            pVencAttr->stAttrH264e.bSingleLumaBuf;
        pInitEncCfg->bSingleEsBuf = pVencAttr->bIsoSendFrmEn ? 0 :
            pHandle->ModParam.stH264eModParam.bSingleEsBuf;
    } else if (pInitEncCfg->codec == CODEC_H265) {
        pInitEncCfg->userDataMaxLength =
            pHandle->ModParam.stH265eModParam.u32UserDataMaxLen;
        pInitEncCfg->bSingleEsBuf = pVencAttr->bIsoSendFrmEn ? 0 :
            pHandle->ModParam.stH265eModParam.bSingleEsBuf;
        if (pHandle->ModParam.stH265eModParam.enRefreshType ==
            H265E_REFRESH_IDR)
            pInitEncCfg->decodingRefreshType = H265_RT_IDR;
        else
            pInitEncCfg->decodingRefreshType = H265_RT_CRA;
    }

    if (pInitEncCfg->bSingleEsBuf) {
        if (pInitEncCfg->codec == CODEC_H264)
            pInitEncCfg->bitstreamBufferSize =
                pHandle->ModParam.stH264eModParam
                    .u32SingleEsBufSize;
        else if (pInitEncCfg->codec == CODEC_H265)
            pInitEncCfg->bitstreamBufferSize =
                pHandle->ModParam.stH265eModParam
                    .u32SingleEsBufSize;
    } else {
        pInitEncCfg->bitstreamBufferSize = pVencAttr->u32BufSize;
    }
    pInitEncCfg->s32ChnNum = pChnHandle->VeChn;
    pInitEncCfg->bEsBufQueueEn = pVencAttr->bEsBufQueueEn;
    pInitEncCfg->bIsoSendFrmEn = pVencAttr->bIsoSendFrmEn;
    pInitEncCfg->s32CmdQueueDepth = pVencAttr->u32CmdQueueDepth;
    if (pInitEncCfg->s32CmdQueueDepth == 0 || pInitEncCfg->s32CmdQueueDepth > MAX_VENC_QUEUE_DEPTH) {
        pInitEncCfg->s32CmdQueueDepth = MAX_VENC_QUEUE_DEPTH;
    }
    pInitEncCfg->s32EncMode = pVencAttr->enEncMode;
    if (pInitEncCfg->s32EncMode < 1 || pInitEncCfg->s32EncMode > 3) {
        pInitEncCfg->s32EncMode = 1;
    }
    pInitEncCfg->u32GopPreset = pVencGopExAttr->u32GopPreset;
    if (pVencGopExAttr->u32GopPreset == GOP_PRESET_IDX_CUSTOM_GOP) {
        memcpy(&pInitEncCfg->gopParam, &pVencGopExAttr->gopParam, sizeof(CustomGopParam));
    }

    pInitEncCfg->s32RotationAngle = pVencAttr->enRotation;
    pInitEncCfg->s32MirrorDirection = pVencAttr->enMirrorDirextion;

    if (pEncCtx->ext.vid.setInitCfgRc)
        pEncCtx->ext.vid.setInitCfgRc(pInitEncCfg, pChnHandle);


    pEncCtx->ext.vid.pHandle = cviVEncOpen(pInitEncCfg);
    if (!pEncCtx->ext.vid.pHandle) {
        CVI_VENC_ERR("cviVEncOpen\n");
        return CVI_ERR_VENC_NULL_PTR;
    }

    return status;
}

static CVI_VOID cviSetInitCfgGop(cviInitEncConfig *pInitEncCfg,
                 venc_chn_context *pChnHandle)
{
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    VENC_GOP_ATTR_S *pga = &pChnAttr->stGopAttr;

    pInitEncCfg->virtualIPeriod = 0;

    if (pga->enGopMode == VENC_GOPMODE_NORMALP) {
        pInitEncCfg->s32IPQpDelta = pga->stNormalP.s32IPQpDelta;

    } else if (pga->enGopMode == VENC_GOPMODE_SMARTP) {
        pInitEncCfg->virtualIPeriod = pInitEncCfg->gop;
        pInitEncCfg->gop = pga->stSmartP.u32BgInterval;
        pInitEncCfg->s32BgQpDelta = pga->stSmartP.s32BgQpDelta;
        pInitEncCfg->s32ViQpDelta = pga->stSmartP.s32ViQpDelta;
    }
}

static CVI_VOID h264e_setInitCfgFixQp(cviInitEncConfig *pInitEncCfg,
                      CVI_VOID *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
    VENC_H264_FIXQP_S *pstH264FixQp = &prcatt->stH264FixQp;

    pInitEncCfg->iqp = pstH264FixQp->u32IQp;
    pInitEncCfg->pqp = pstH264FixQp->u32PQp;
    pInitEncCfg->gop = pstH264FixQp->u32Gop;
    pInitEncCfg->framerate = (int)pstH264FixQp->fr32DstFrameRate;

    cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_VOID h264e_setInitCfgCbr(cviInitEncConfig *pInitEncCfg,
                    CVI_VOID *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
    VENC_H264_CBR_S *pstH264Cbr = &prcatt->stH264Cbr;

    pInitEncCfg->statTime = pstH264Cbr->u32StatTime;
    pInitEncCfg->gop = pstH264Cbr->u32Gop;
    pInitEncCfg->bitrate = pstH264Cbr->u32BitRate;
    pInitEncCfg->framerate = (int)pstH264Cbr->fr32DstFrameRate;

    cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_VOID h264e_setInitCfgVbr(cviInitEncConfig *pInitEncCfg,
                    CVI_VOID *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
    VENC_H264_VBR_S *pstH264Vbr = &prcatt->stH264Vbr;
    VENC_RC_PARAM_S *prcparam = &pChnHandle->rcParam;

    pInitEncCfg->statTime = pstH264Vbr->u32StatTime;
    pInitEncCfg->gop = pstH264Vbr->u32Gop;
    pInitEncCfg->bitrate = pstH264Vbr->u32MaxBitRate;
    pInitEncCfg->maxbitrate = pstH264Vbr->u32MaxBitRate;
    pInitEncCfg->framerate = (int)pstH264Vbr->fr32DstFrameRate;
    pInitEncCfg->s32ChangePos = prcparam->stParamH264Vbr.s32ChangePos;

    cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_VOID h264e_setInitCfgAVbr(cviInitEncConfig *pInitEncCfg,
                     CVI_VOID *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
    VENC_H264_AVBR_S *pstH264AVbr = &prcatt->stH264AVbr;
    VENC_PARAM_H264_AVBR_S *pprc = &pChnHandle->rcParam.stParamH264AVbr;

    pInitEncCfg->statTime = pstH264AVbr->u32StatTime;
    pInitEncCfg->gop = pstH264AVbr->u32Gop;
    pInitEncCfg->framerate = (int)pstH264AVbr->fr32DstFrameRate;
    pInitEncCfg->bitrate = pstH264AVbr->u32MaxBitRate;
    pInitEncCfg->maxbitrate = pstH264AVbr->u32MaxBitRate;
    pInitEncCfg->s32ChangePos = pprc->s32ChangePos;
    pInitEncCfg->s32MinStillPercent = pprc->s32MinStillPercent;
    pInitEncCfg->u32MaxStillQP = pprc->u32MaxStillQP;
    pInitEncCfg->u32MotionSensitivity = pprc->u32MotionSensitivity;
    pInitEncCfg->s32AvbrFrmLostOpen = pprc->s32AvbrFrmLostOpen;
    pInitEncCfg->s32AvbrFrmGap = pprc->s32AvbrFrmGap;
    pInitEncCfg->s32AvbrPureStillThr = pprc->s32AvbrPureStillThr;

    cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_VOID h264e_setInitCfgUbr(cviInitEncConfig *pInitEncCfg,
                    CVI_VOID *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
    VENC_H264_UBR_S *pstH264Ubr = &prcatt->stH264Ubr;

    pInitEncCfg->statTime = pstH264Ubr->u32StatTime;
    pInitEncCfg->gop = pstH264Ubr->u32Gop;
    pInitEncCfg->bitrate = pstH264Ubr->u32BitRate;
    pInitEncCfg->framerate = (int)pstH264Ubr->fr32DstFrameRate;

    cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_S32 h264e_mapNaluType(VENC_PACK_S *ppack, CVI_S32 cviNalType)
{
    int h264naluType[] = {
        H264E_NALU_ISLICE,
        H264E_NALU_PSLICE,
        H264E_NALU_BSLICE,
        H264E_NALU_BUTT,
        H264E_NALU_IDRSLICE,
        H264E_NALU_SPS,
        H264E_NALU_PPS,
        H264E_NALU_SEI,
    };
    int naluType;

    if (!ppack) {
        CVI_VENC_ERR("ppack is NULL\n");
        return -1;
    }

    if (!ppack->pu8Addr) {
        CVI_VENC_ERR("ppack->pu8Addr is NULL\n");
        return -1;
    }

    naluType = ppack->pu8Addr[4] & 0x1f;

    if (cviNalType < NAL_NONE || cviNalType >= NAL_MAX) {
        CVI_VENC_ERR("cviNalType = %d\n", cviNalType);
        return -1;
    }

    if (naluType == H264_NALU_TYPE_IDR)
        ppack->DataType.enH264EType = H264E_NALU_IDRSLICE;
    else
        ppack->DataType.enH264EType = h264naluType[cviNalType];

    CVI_VENC_DBG("enH264EType = %d\n", ppack->DataType.enH264EType);
    return 0;
}

static CVI_VOID h265e_setInitCfgFixQp(cviInitEncConfig *pInitEncCfg,
                      CVI_VOID *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
    VENC_H265_FIXQP_S *pstH265FixQp = &prcatt->stH265FixQp;

    pInitEncCfg->iqp = pstH265FixQp->u32IQp;
    pInitEncCfg->pqp = pstH265FixQp->u32PQp;
    pInitEncCfg->gop = pstH265FixQp->u32Gop;
    pInitEncCfg->framerate = (int)pstH265FixQp->fr32DstFrameRate;
    cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_VOID h265e_setInitCfgCbr(cviInitEncConfig *pInitEncCfg,
                    CVI_VOID *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
    VENC_H265_CBR_S *pstH265Cbr = &prcatt->stH265Cbr;

    pInitEncCfg->statTime = pstH265Cbr->u32StatTime;
    pInitEncCfg->gop = pstH265Cbr->u32Gop;
    pInitEncCfg->bitrate = pstH265Cbr->u32BitRate;
    pInitEncCfg->framerate = (int)pstH265Cbr->fr32DstFrameRate;
    cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_VOID h265e_setInitCfgVbr(cviInitEncConfig *pInitEncCfg,
                    CVI_VOID *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
    VENC_H265_VBR_S *pstH265Vbr = &prcatt->stH265Vbr;
    VENC_RC_PARAM_S *prcparam = &pChnHandle->rcParam;

    pInitEncCfg->statTime = pstH265Vbr->u32StatTime;
    pInitEncCfg->gop = pstH265Vbr->u32Gop;
    pInitEncCfg->bitrate = pstH265Vbr->u32MaxBitRate;
    pInitEncCfg->maxbitrate = pstH265Vbr->u32MaxBitRate;
    pInitEncCfg->s32ChangePos = prcparam->stParamH265Vbr.s32ChangePos;
    pInitEncCfg->framerate = (int)pstH265Vbr->fr32DstFrameRate;
    cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_VOID h265e_setInitCfgAVbr(cviInitEncConfig *pInitEncCfg,
                     CVI_VOID *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
    VENC_H265_AVBR_S *pstH265AVbr = &prcatt->stH265AVbr;
    VENC_PARAM_H265_AVBR_S *pprc = &pChnHandle->rcParam.stParamH265AVbr;

    pInitEncCfg->statTime = pstH265AVbr->u32StatTime;
    pInitEncCfg->gop = pstH265AVbr->u32Gop;
    pInitEncCfg->framerate = (int)pstH265AVbr->fr32DstFrameRate;
    pInitEncCfg->bitrate = pstH265AVbr->u32MaxBitRate;
    pInitEncCfg->maxbitrate = pstH265AVbr->u32MaxBitRate;
    pInitEncCfg->s32ChangePos = pprc->s32ChangePos;
    pInitEncCfg->s32MinStillPercent = pprc->s32MinStillPercent;
    pInitEncCfg->u32MaxStillQP = pprc->u32MaxStillQP;
    pInitEncCfg->u32MotionSensitivity = pprc->u32MotionSensitivity;
    pInitEncCfg->s32AvbrFrmLostOpen = pprc->s32AvbrFrmLostOpen;
    pInitEncCfg->s32AvbrFrmGap = pprc->s32AvbrFrmGap;
    pInitEncCfg->s32AvbrPureStillThr = pprc->s32AvbrPureStillThr;
    cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_VOID h265e_setInitCfgQpMap(cviInitEncConfig *pInitEncCfg,
                      CVI_VOID *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
    VENC_H265_QPMAP_S *pstH265QpMap = &prcatt->stH265QpMap;

    pInitEncCfg->statTime = pstH265QpMap->u32StatTime;
    pInitEncCfg->gop = pstH265QpMap->u32Gop;
    pInitEncCfg->bitrate =
        DEAULT_QP_MAP_BITRATE; // QpMap uses CBR as basic settings
    pInitEncCfg->framerate = (int)pstH265QpMap->fr32DstFrameRate;
    cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_VOID h265e_setInitCfgUbr(cviInitEncConfig *pInitEncCfg,
                    CVI_VOID *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
    VENC_H265_UBR_S *pstH265Ubr = &prcatt->stH265Ubr;

    pInitEncCfg->statTime = pstH265Ubr->u32StatTime;
    pInitEncCfg->gop = pstH265Ubr->u32Gop;
    pInitEncCfg->bitrate = pstH265Ubr->u32BitRate;
    pInitEncCfg->framerate = (int)pstH265Ubr->fr32DstFrameRate;

    cviSetInitCfgGop(pInitEncCfg, pChnHandle);
}

static CVI_S32 h265e_mapNaluType(VENC_PACK_S *ppack, CVI_S32 cviNalType)
{
    int h265naluType[] = {
        H265E_NALU_ISLICE,
        H265E_NALU_PSLICE,
        H265E_NALU_BSLICE,
        H265E_NALU_BUTT,
        H265E_NALU_IDRSLICE,
        H265E_NALU_SPS,
        H265E_NALU_PPS,
        H265E_NALU_SEI,
        H265E_NALU_VPS,
    };
    int naluType;

    if (!ppack) {
        CVI_VENC_ERR("ppack is NULL\n");
        return -1;
    }

    if (!ppack->pu8Addr) {
        CVI_VENC_ERR("ppack->pu8Addr is NULL\n");
        return -1;
    }

    naluType = (ppack->pu8Addr[4] & 0x7f) >> 1;

    if (cviNalType < NAL_NONE || cviNalType >= NAL_MAX) {
        CVI_VENC_ERR("cviNalType = %d\n", cviNalType);
        return -1;
    }

    if (naluType == H265_NALU_TYPE_W_RADL ||
        naluType == H265_NALU_TYPE_N_LP)
        ppack->DataType.enH265EType = H265E_NALU_IDRSLICE;
    else
        ppack->DataType.enH265EType = h265naluType[cviNalType];

    CVI_VENC_DBG("enH265EType = %d\n", ppack->DataType.enH265EType);
    return 0;
}

static CVI_S32 vidEnc_close(CVI_VOID *ctx)
{
    CVI_S32 status = CVI_SUCCESS;
    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;

    if (pEncCtx->ext.vid.pHandle) {
        status = cviVEncClose(pEncCtx->ext.vid.pHandle);
        if (status < 0) {
            CVI_VENC_ERR("cviVEncClose, status = %d\n", status);
            return status;
        }
    }

    return status;
}



static CVI_S32 vidEnc_enc_one_pic(CVI_VOID *ctx,
                  const VIDEO_FRAME_INFO_S *pstFrame,
                  CVI_S32 s32MilliSec)
{
    CVI_S32 status = CVI_SUCCESS;
    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;
    cviEncOnePicCfg encOnePicCfg;
    CVI_U8 mtable[MO_TBL_SIZE];
    cviEncOnePicCfg *pPicCfg = &encOnePicCfg;
    CVIFRAMEBUF srcInfo, *psi = &srcInfo;
    struct vb_s *blk = (struct vb_s *)pstFrame->stVFrame.pPrivateData;

    setSrcInfo(psi, pEncCtx, pstFrame);

    pPicCfg->addrY = psi->vbY.virt_addr;
    pPicCfg->addrCb = psi->vbCb.virt_addr;
    pPicCfg->addrCr = psi->vbCr.virt_addr;

    pPicCfg->phyAddrY = psi->vbY.phys_addr;
    pPicCfg->phyAddrCb = psi->vbCb.phys_addr;
    pPicCfg->phyAddrCr = psi->vbCr.phys_addr;
    pPicCfg->u64Pts = pstFrame->stVFrame.u64PTS;
    pPicCfg->src_end = pstFrame->stVFrame.bSrcEnd;
    pPicCfg->src_idx = pstFrame->stVFrame.s32FrameIdx;
    pPicCfg->stride = psi->strideY;
    switch (pstFrame->stVFrame.enPixelFormat) {
    case PIXEL_FORMAT_NV12:
        pPicCfg->cbcrInterleave = 1;
        pPicCfg->nv21 = 0;
        break;
    case PIXEL_FORMAT_NV21:
        pPicCfg->cbcrInterleave = 1;
        pPicCfg->nv21 = 1;
        break;
    case PIXEL_FORMAT_YUV_PLANAR_420:
    default:
        pPicCfg->cbcrInterleave = 0;
        pPicCfg->nv21 = 0;
        break;
    }

    if (pEncCtx->base.rcMode == RC_MODE_AVBR) {
        if (blk) {
            pPicCfg->picMotionLevel = blk->buf.motion_lv;
            pPicCfg->picMotionMap = mtable;
            pPicCfg->picMotionMapSize = MO_TBL_SIZE;
            memcpy(mtable, blk->buf.motion_table, MO_TBL_SIZE);
        }
        pPicCfg->picMotionMap = mtable;
        //cviCopyMotionMap(pEncCtx->ext.vid.pHandle, pPicCfg, pEncCtx->ext.vid.pHandle);
    }

    status = cviVEncEncOnePic(pEncCtx->ext.vid.pHandle, pPicCfg,
                  s32MilliSec);

    if (status == ENC_TIMEOUT || status == RETCODE_QUEUEING_FAILURE || status == RETCODE_STREAM_BUF_FULL) {
        CVI_VENC_WARN("cviVEncEncOnePic, status = %d\n", status);
        return CVI_ERR_VENC_BUSY;
    }

    return status;
}

static CVI_S32 vidEnc_get_stream(CVI_VOID *ctx, VENC_STREAM_S *pstStream,
                 CVI_S32 s32MilliSec)
{
    CVI_S32 status = CVI_SUCCESS;
    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;
    cviVEncStreamInfo *pStreamInfo = &pEncCtx->ext.vid.streamInfo;
    Queue *psp = NULL;
    VENC_PACK_S *ppack = NULL;
    stPack *pqpacks = NULL;
    CVI_U32 idx = 0;
    CVI_S32 totalPacks = 0;
    CVI_U32 encHwTimeus = 0;

    status = cviVEncGetStream(pEncCtx->ext.vid.pHandle, pStreamInfo,
                  s32MilliSec);
    CVI_VENC_INFO("cviVEncGetStream status %d\n", status);
    if (status == ENC_TIMEOUT) {
        return CVI_ERR_VENC_BUSY;
    } else if (status) {
        CVI_VENC_ERR("get stream failed,status %d\n", status);
        return CVI_ERR_VENC_INVALILD_RET;
    }

    psp = (Queue *)pStreamInfo->psp;
    if (!psp) {
        CVI_VENC_ERR("psp is null\n");
        return CVI_ERR_VENC_NULL_PTR;
    }

    totalPacks = Queue_Get_Cnt(psp);
    if (totalPacks == 0) {
        return CVI_ERR_VENC_GET_STREAM_END;
    }

    pstStream->u32PackCount = 0;
    for (idx = 0; (idx < totalPacks) && (idx < MAX_NUM_PACKS); idx++) {
        ppack = &pstStream->pstPack[idx];

        if (!ppack) {
            CVI_VENC_ERR("get NULL pack, uPackCount:%d idx:%d\n", pstStream->u32PackCount, idx);
            break;
        }

        memset(ppack, 0, sizeof(VENC_PACK_S));
        pqpacks = Queue_Dequeue(psp);
        if (!pqpacks) {
            continue;
        }

        // psp->pack[idx].bUsed = CVI_TRUE;
        ppack->u64PhyAddr = pqpacks->u64PhyAddr;
        ppack->pu8Addr = pqpacks->addr;
        ppack->u32Len = pqpacks->len;
        ppack->u64PTS = pqpacks->u64PTS;
        ppack->u64DTS = pqpacks->u64DTS;
        ppack->releasFrameIdx = pqpacks->encSrcIdx;
        ppack->u64CustomMapPhyAddr = pqpacks->u64CustomMapAddr;
        ppack->u32AvgCtuQp = pqpacks->u32AvgCtuQp;
        encHwTimeus = pqpacks->u32EncHwTime;
        status = pEncCtx->ext.vid.mapNaluType(
            ppack, pqpacks->cviNalType);
        if (status) {
            CVI_VENC_ERR("mapNaluType, status = %d\n", status);
            return status;
        }
        pstStream->u32PackCount++;
    }
    pEncCtx->base.u64EncHwTime = (CVI_U64)encHwTimeus;

    return status;
}

static CVI_S32 vidEnc_release_stream(CVI_VOID *ctx, VENC_STREAM_S *pstStream)
{
    CVI_S32 status = CVI_SUCCESS;
    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;
    stPack vencPack[MAX_NUM_PACKS] = {0};
    CVI_S32 idx = 0;

    for (idx = 0; (idx < pstStream->u32PackCount) && (idx < MAX_NUM_PACKS); idx++) {
        vencPack[idx].u64PhyAddr = pstStream->pstPack[idx].u64PhyAddr;
        vencPack[idx].addr = pstStream->pstPack[idx].pu8Addr;
        vencPack[idx].len = pstStream->pstPack[idx].u32Len;
        if ( pstStream->pstPack[idx].DataType.enH264EType == H264E_NALU_SEI
            || pstStream->pstPack[idx].DataType.enH265EType == H265E_NALU_SEI) {
            vencPack[idx].cviNalType = NAL_SEI;
        }
    }

    status = cviVEncReleaseStream(pEncCtx->ext.vid.pHandle, vencPack, pstStream->u32PackCount);
    if (status != CVI_SUCCESS) {
        CVI_VENC_ERR("cviVEncReleaseStream, status = %d\n", status);
        return status;
    }

    return status;
}

static CVI_S32 vidEnc_ioctl(CVI_VOID *ctx, CVI_S32 op, CVI_VOID *arg)
{
    CVI_S32 status = CVI_SUCCESS;
    venc_enc_ctx *pEncCtx = (venc_enc_ctx *)ctx;
    CVI_S32 currOp;

    currOp = (op & CVI_H26X_OP_MASK) >> CVI_H26X_OP_SHIFT;
    if (currOp == 0) {
        CVI_VENC_WARN("op = 0x%X, currOp = 0x%X\n", op, currOp);
        return 0;
    }

    status = cviVEncIoctl(pEncCtx->ext.vid.pHandle, currOp, arg);
    if (status != CVI_SUCCESS) {
        CVI_VENC_ERR("cviVEncIoctl, currOp = 0x%X, status = %d\n",
                 currOp, status);
        return status;
    }

    return status;
}


CVI_S32 venc_create_enc_ctx(venc_enc_ctx *pEncCtx, CVI_VOID *pchnctx)
{
    venc_chn_context *pChnHandle = (venc_chn_context *)pchnctx;
    VENC_CHN_ATTR_S *pChnAttr = pChnHandle->pChnAttr;
    VENC_ATTR_S *pVencAttr = &pChnAttr->stVencAttr;
    VENC_RC_ATTR_S *prcatt = &pChnAttr->stRcAttr;
    CVI_S32 status = 0;

    VENC_MEMSET(pEncCtx, 0, sizeof(venc_enc_ctx));

    switch (pVencAttr->enType) {
    case PT_JPEG:
    case PT_MJPEG:
        pEncCtx->base.init = &jpege_init;
        pEncCtx->base.open = &jpege_open;
        pEncCtx->base.close = &jpege_close;
        pEncCtx->base.encOnePic = &jpege_enc_one_pic;
        pEncCtx->base.getStream = &jpege_get_stream;
        pEncCtx->base.releaseStream = &jpege_release_stream;
        pEncCtx->base.ioctl = &jpege_ioctl;
        break;
    case PT_H264:
        pEncCtx->base.init = &vidEnc_init;
        pEncCtx->base.open = &vidEnc_open;
        pEncCtx->base.close = &vidEnc_close;
        pEncCtx->base.encOnePic = &vidEnc_enc_one_pic;
        pEncCtx->base.getStream = &vidEnc_get_stream;
        pEncCtx->base.releaseStream = &vidEnc_release_stream;
        pEncCtx->base.ioctl = &vidEnc_ioctl;

        pEncCtx->ext.vid.setInitCfgFixQp = h264e_setInitCfgFixQp;
        pEncCtx->ext.vid.setInitCfgCbr = h264e_setInitCfgCbr;
        pEncCtx->ext.vid.setInitCfgVbr = h264e_setInitCfgVbr;
        pEncCtx->ext.vid.setInitCfgAVbr = h264e_setInitCfgAVbr;
        pEncCtx->ext.vid.setInitCfgUbr = h264e_setInitCfgUbr;
        pEncCtx->ext.vid.mapNaluType = h264e_mapNaluType;

        pEncCtx->base.u32Profile = pVencAttr->u32Profile;
        pEncCtx->base.rcMode = prcatt->enRcMode - VENC_RC_MODE_H264CBR;

        if (pEncCtx->base.rcMode == RC_MODE_CBR) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgCbr;
            pEncCtx->base.bVariFpsEn = prcatt->stH264Cbr.bVariFpsEn;
        } else if (pEncCtx->base.rcMode == RC_MODE_VBR) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgVbr;
            pEncCtx->base.bVariFpsEn = prcatt->stH264Vbr.bVariFpsEn;
        } else if (pEncCtx->base.rcMode == RC_MODE_AVBR) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgAVbr;
            pEncCtx->base.bVariFpsEn =
                prcatt->stH264AVbr.bVariFpsEn;
        } else if (pEncCtx->base.rcMode == RC_MODE_FIXQP) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgFixQp;
            pEncCtx->base.bVariFpsEn =
                prcatt->stH264FixQp.bVariFpsEn;
        } else if (pEncCtx->base.rcMode == RC_MODE_UBR) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgUbr;
            pEncCtx->base.bVariFpsEn = prcatt->stH264Ubr.bVariFpsEn;
        }
        break;
    case PT_H265:
        pEncCtx->base.init = &vidEnc_init;
        pEncCtx->base.open = &vidEnc_open;
        pEncCtx->base.close = &vidEnc_close;
        pEncCtx->base.encOnePic = &vidEnc_enc_one_pic;
        pEncCtx->base.getStream = &vidEnc_get_stream;
        pEncCtx->base.releaseStream = &vidEnc_release_stream;
        pEncCtx->base.ioctl = &vidEnc_ioctl;

        pEncCtx->ext.vid.setInitCfgFixQp = h265e_setInitCfgFixQp;
        pEncCtx->ext.vid.setInitCfgCbr = h265e_setInitCfgCbr;
        pEncCtx->ext.vid.setInitCfgVbr = h265e_setInitCfgVbr;
        pEncCtx->ext.vid.setInitCfgAVbr = h265e_setInitCfgAVbr;
        pEncCtx->ext.vid.setInitCfgQpMap = h265e_setInitCfgQpMap;
        pEncCtx->ext.vid.setInitCfgUbr = h265e_setInitCfgUbr;
        pEncCtx->ext.vid.mapNaluType = h265e_mapNaluType;

        pEncCtx->base.u32Profile = pVencAttr->u32Profile;
        pEncCtx->base.rcMode = prcatt->enRcMode - VENC_RC_MODE_H265CBR;

        if (pEncCtx->base.rcMode == RC_MODE_CBR) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgCbr;
            pEncCtx->base.bVariFpsEn = prcatt->stH265Cbr.bVariFpsEn;
        } else if (pEncCtx->base.rcMode == RC_MODE_VBR) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgVbr;
            pEncCtx->base.bVariFpsEn = prcatt->stH265Vbr.bVariFpsEn;
        } else if (pEncCtx->base.rcMode == RC_MODE_AVBR) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgAVbr;
            pEncCtx->base.bVariFpsEn =
                prcatt->stH265AVbr.bVariFpsEn;
        } else if (pEncCtx->base.rcMode == RC_MODE_FIXQP) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgFixQp;
            pEncCtx->base.bVariFpsEn =
                prcatt->stH265FixQp.bVariFpsEn;
        } else if (pEncCtx->base.rcMode == RC_MODE_QPMAP) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgQpMap;
            pEncCtx->base.bVariFpsEn =
                prcatt->stH265QpMap.bVariFpsEn;
        } else if (pEncCtx->base.rcMode == RC_MODE_UBR) {
            pEncCtx->ext.vid.setInitCfgRc =
                pEncCtx->ext.vid.setInitCfgUbr;
            pEncCtx->base.bVariFpsEn = prcatt->stH265Ubr.bVariFpsEn;
        }
        break;
    default:
        CVI_VENC_ERR("enType = %d\n", pVencAttr->enType);
        return -1;
    }

    pEncCtx->base.width = pVencAttr->u32PicWidth;
    pEncCtx->base.height = pVencAttr->u32PicHeight;

    return status;
}


