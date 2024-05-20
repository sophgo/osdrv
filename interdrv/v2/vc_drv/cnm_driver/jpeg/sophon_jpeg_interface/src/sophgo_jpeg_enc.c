#include "sophgo_jpeg_internal.h"
#include "jputypes.h"
#include "jdi.h"
#include "jpu_helper.h"
#include "drv_file.h"
#include "regdefine.h"

#define BS_SIZE_ALIGNMENT           4096
#define MIN_BS_SIZE                 8192
#define ENC_SRC_BUF_NUM             1

//for buffer full case
typedef struct {
    jpu_buffer_t        vbBitstreamExt;
    size_t              length;
}JPEG_BS_DATA_STREAM_INFO;


typedef struct {
    JpgEncHandle        pHandle;
    jpu_buffer_t        vbBitstream;
    JPEG_BS_DATA_STREAM_INFO stBsFullInfo;
    JpgEncOpenParam     stJpgEncOp;
    FrameBuffer         frameBuf[MAX_FRAME_JPU];
    FRAME_BUF           *pFrame[MAX_FRAME_JPU];
    int                 encHeaderMode;
    int                 iCoreIdx;
    JpgEncParamSet      encHeaderParam;
    jpu_buffer_t        vbHeaderBitstream;
    JpgEncOutputInfo    outputInfo;
    EncConfigParam      *pstEncConfig;
    int                 i32ValidCnt;
}SOPHGO_JPEG_ENC_HANDLE_INFO,*SOPHGO_JPEG_ENC_HANDLE_INFO_PTR;


void dump_enc_yuv(SOPHGO_S_FRAME_BUF *pstFrame)
{
    int i = 0;
    int writeLen = 0;
    int writeTotalLen = 0;
    int datLen = pstFrame->s32Width;
    unsigned char *addrY_v = (unsigned char *)(pstFrame->vbY.virt_addr);
    unsigned char *addrCb_v = (unsigned char *)(pstFrame->vbCb.virt_addr);
    unsigned char *addrCr_v = (unsigned char *)(pstFrame->vbCr.virt_addr);
    unsigned char *address = addrY_v;
    int iChromaHeight;

    drv_file_t *fpYuv = drv_fopen("/mnt/sd/enc_dump.yuv", "wb");
    if (0 == fpYuv) {
        JLOG(ERR, "Cann't create a file to write data\n");
        return ;
    }

    for (i = 0; i < pstFrame->s32Height; i++) {
        writeLen = drv_fwrite(address,
                  sizeof(unsigned char), datLen, fpYuv);
        writeTotalLen += writeLen;
        address = address + pstFrame->s32StrideY;
    }

    iChromaHeight = pstFrame->s32Height>>1;
    if(pstFrame->eFormat == YUV_E_FORMAT_422) {
        iChromaHeight = iChromaHeight*2;
    }

    address = addrCb_v;
    writeLen = drv_fwrite((unsigned char *)address, sizeof(unsigned char),
              iChromaHeight * pstFrame->s32StrideC, fpYuv);

    address = addrCr_v;

    writeLen = drv_fwrite((unsigned char *)address, sizeof(unsigned char),
              iChromaHeight * pstFrame->s32StrideC, fpYuv);
    if (0 != fpYuv) {
        drv_fclose(fpYuv);
        fpYuv = NULL;
    }

    return;
}

static void sopProcessBsFull(SOPHGO_JPEG_HANDLE_PTR *pvJpgHandle)
{
    jpu_buffer_t        vbBitstream;
    SOPHGO_JPEG_ENC_HANDLE_INFO *pjpgHandle = (SOPHGO_JPEG_ENC_HANDLE_INFO *)pvJpgHandle;
    JPEG_BS_DATA_STREAM_INFO *pstBsFullInfo = &pjpgHandle->stBsFullInfo;
    PhysicalAddress rdPtr;
    PhysicalAddress wrPtr;
    int bsSize;

    //allocate new ion
    vbBitstream.size = pstBsFullInfo->length + pjpgHandle->vbBitstream.size * 2;
    vbBitstream.is_cached = 0;
    if (jdi_allocate_dma_memory(&vbBitstream) < 0) {
        JLOG(ERR, "sopProcessBsFull fail to allocate bitstream buffer\n" );
        return ;
    }

    if (jdi_invalidate_cache(&vbBitstream) < 0) {
        JLOG(ERR, "fail to invalidate bitstream buffer cache addr:%lx, size:%ld\n", vbBitstream.phys_addr, vbBitstream.size);
        return;
    }

    JPU_EncGetBitstreamBuffer(pjpgHandle->pHandle, &rdPtr, &wrPtr, &bsSize);
    //copy old bs data and current bs data
    if(pstBsFullInfo->length)
        memcpy((void *)vbBitstream.virt_addr, (void *)pstBsFullInfo->vbBitstreamExt.virt_addr,
            pstBsFullInfo->length);
    memcpy((void *)(vbBitstream.virt_addr + pstBsFullInfo->length),
        (void *)pjpgHandle->vbBitstream.virt_addr, bsSize);
    pstBsFullInfo->length += bsSize;

    //free old bs ion
    if(pstBsFullInfo->vbBitstreamExt.base)
        jdi_free_dma_memory(&pstBsFullInfo->vbBitstreamExt);

    //set new bs info to stBsFullInfo
    pstBsFullInfo->vbBitstreamExt = vbBitstream;
    JLOG(INFO,"sopProcessBsFull length:%ld\n", pstBsFullInfo->length);

    return ;
}

static void sopCalcSliceHeight(JpgEncOpenParam* encOP, Uint32 sliceHeight)
{
    Uint32 width    = encOP->picWidth;
    Uint32 height   = encOP->picHeight;
    Uint32 aligned_buf_height;
    FrameFormat format = encOP->sourceFormat;

    if (encOP->rotation == 90 || encOP->rotation == 270) {
        width  = encOP->picHeight;
        height = encOP->picWidth;
        if (format == FORMAT_422) format = FORMAT_440;
        else if (format == FORMAT_440) format = FORMAT_422;
    }

    if (format == FORMAT_420 || format == FORMAT_440)
        aligned_buf_height = JPU_CEIL(16, height);
    else
        aligned_buf_height = JPU_CEIL(8, height);

    if (sliceHeight == 0) {
        if (format == FORMAT_420 || format == FORMAT_440)
            encOP->sliceHeight = aligned_buf_height;
        else
            encOP->sliceHeight = aligned_buf_height;
    }
    else
        encOP->sliceHeight = sliceHeight;

    if (encOP->sliceHeight != aligned_buf_height) {
        if (format == FORMAT_420 || format == FORMAT_422)
            encOP->restartInterval = (width+15)/16;
        else
            encOP->restartInterval = (width+7)/8;

        if (format == FORMAT_420 || format == FORMAT_440)
            encOP->restartInterval *= (encOP->sliceHeight/16);
        else
            encOP->restartInterval *= (encOP->sliceHeight/8);
        encOP->sliceInstMode = TRUE;
    }
    JLOG(INFO,"sliceInstMode:%d sliceHeight:%d aligh:%d\n",
        encOP->sliceInstMode, encOP->sliceHeight, aligned_buf_height);
}

int sophgo_jpeg_enc_set_quality(SOPHGO_JPEG_HANDLE_PTR *pvJpgHandle, void *pvData)
{
    EncConfigParam *pstEncConfig;
    SOPHGO_JPEG_ENC_HANDLE_INFO *pjpgHandle = (SOPHGO_JPEG_ENC_HANDLE_INFO *)pvJpgHandle;
    JpgQualityTable *pjpgQtTab = (JpgQualityTable *) pvData;
    CHECK_PTR_RET_INT(pvJpgHandle);
    pstEncConfig = pjpgHandle->pstEncConfig;

    if(!pjpgQtTab) {
        JLOG(ERR,"data is NULL.\n");
        return JPG_RET_INVALID_PARAM;
    }

    if(!pstEncConfig) {
        JLOG(ERR,"pstEncConfig is NULL.\n");
        return JPG_RET_WRONG_CALL_SEQUENCE;
    }

    if(pjpgQtTab->quality > 100) {
        JLOG(ERR,"quality %u ,only ranges between 0-100,\n", pjpgQtTab->quality);
        return JPG_RET_INVALID_PARAM;
    }

    // set quality table by self
    if (pjpgQtTab->quality == 50) {
        JPU_EncGiveCommand(pjpgHandle->pHandle, SET_JPG_QUALITY_TABLE, pjpgQtTab);
        JLOG(INFO,"set jpeg's qt tab suc.\n");
        return JPG_RET_SUCCESS;
    }

    if (pjpgQtTab->quality > 0)  {
        pstEncConfig->encQualityPercentage = pjpgQtTab->quality;
        JPU_EncGiveCommand(pjpgHandle->pHandle, SET_JPG_QUALITY_FACTOR, &pstEncConfig->encQualityPercentage);
        JLOG(INFO,"set jpeg's qulity suc.\n");
    }

    return JPG_RET_SUCCESS;
}

int sophgo_jpeg_enc_get_quality(SOPHGO_JPEG_HANDLE_PTR *pvJpgHandle, void *pvData)
{
    EncConfigParam *pstEncConfig;
    SOPHGO_JPEG_ENC_HANDLE_INFO *pjpgHandle = (SOPHGO_JPEG_ENC_HANDLE_INFO *)pvJpgHandle;
    JpgQualityTable *pjpgQtTab = (JpgQualityTable *) pvData;
    CHECK_PTR_RET_INT(pvJpgHandle);
    pstEncConfig = pjpgHandle->pstEncConfig;

    if(!pstEncConfig) {
        JLOG(ERR,"pstEncConfig is NULL.\n");
        return JPG_RET_WRONG_CALL_SEQUENCE;
    }

    if(!pvData) {
        JLOG(ERR,"data is NULL.\n");
        return JPG_RET_INVALID_PARAM;
    }

    pjpgQtTab->quality = pstEncConfig->encQualityPercentage;
    JPU_EncGiveCommand(pjpgHandle->pHandle, GET_JPG_QUALITY_TABLE, pjpgQtTab);
    JLOG(INFO,"get jpeg's qulity suc.\n");

    return JPG_RET_SUCCESS;
}


/* ENC */
int sophgo_jpeg_enc_open_instance(SOPHGO_JPEG_HANDLE_PTR *pvJpgHandle, SOPHGO_S_ENC_CONFIG_PARAM stConfig)
{
    JpgEncOpenParam *pstJpgEncOp;
    JpgRet ret = JPG_RET_SUCCESS;
    EncConfigParam *pstEncConfig;
    JpgEncHandle handle        = { 0 };
    JpgInst *pJpgInst = 0;
    int instIdx = 0;
    int                 srcFrameFormat = 0;
    int                 framebufWidth = 0, framebufHeight = 0;
    Uint32              needFrameBufCount;
    Uint32              bitDepth    = 8;
    int                 i;
    JpgEncInfo          *pEncInfo = 0;
    SOPHGO_JPEG_ENC_HANDLE_INFO_PTR jpgHandle;

    jpgHandle = kzalloc(sizeof(SOPHGO_JPEG_ENC_HANDLE_INFO), GFP_KERNEL);
    if(!jpgHandle)
        return JPG_RET_FAILURE;

    pstJpgEncOp = vmalloc(sizeof(JpgEncOpenParam));
    if (pstJpgEncOp == NULL) {
        JLOG(ERR, "no memory for pstJpgEncOp\n");
        return JPG_RET_FAILURE;
    }

    pstEncConfig = vmalloc(sizeof(EncConfigParam));
    if (pstEncConfig == NULL) {
        JLOG(ERR, "no memory for pstEncConfig\n");
        return JPG_RET_FAILURE;
    }
    memset(pstEncConfig, 0x0, sizeof(EncConfigParam));

    pstEncConfig->picWidth = stConfig.s32PicWidth;
    pstEncConfig->picHeight = stConfig.s32PicHeight;
    pstEncConfig->sourceSubsample = stConfig.eSourceFormat;
    pstEncConfig->packedFormat = stConfig.ePackedFormat;
    pstEncConfig->chromaInterleave = stConfig.eChromaInterleave;
    pstEncConfig->rotation = stConfig.s32RotAngle;
    pstEncConfig->mirror = (JpgMirrorDirection)stConfig.s32MirDir;
    if(!stConfig.s32BitstreamBufSize) {
        stConfig.s32BitstreamBufSize = STREAM_BUF_SIZE;
    }
    JLOG(INFO, "format:%d packed:%d Width:%d Height:%d\n", stConfig.eSourceFormat,
        stConfig.ePackedFormat, stConfig.s32PicWidth, stConfig.s32PicHeight);
    pstEncConfig->bsSize = stConfig.s32BitstreamBufSize - stConfig.s32BitstreamBufSize%BS_SIZE_ALIGNMENT;
    // pstEncConfig config

    memset(pstJpgEncOp, 0, sizeof(JpgEncOpenParam));
    if (getJpgEncOpenParamDefault(pstJpgEncOp, pstEncConfig) == FALSE) {
        JLOG(ERR, "getJpgEncOpenParamDefault falied\n");
        goto ERR_ENC_INIT;
    }

    if ((pstEncConfig->bsSize%BS_SIZE_ALIGNMENT) != 0 || pstEncConfig->bsSize < MIN_BS_SIZE) {
        JLOG(ERR, "Invalid size of bitstream buffer %u\n", pstEncConfig->bsSize);
        goto ERR_ENC_INIT;
    }

    jpgHandle->vbBitstream.size = pstEncConfig->bsSize;
    jpgHandle->vbBitstream.is_cached = 0;
    if (jdi_allocate_dma_memory(&jpgHandle->vbBitstream) < 0) {
        JLOG(ERR, "fail to allocate bitstream buffer\n" );
        goto ERR_ENC_INIT;
    }

    if (jdi_invalidate_cache(&jpgHandle->vbBitstream) < 0) {
        JLOG(ERR, "fail to invalidate bitstream buffer\n");
        goto ERR_ENC_INIT;
    }
    pstJpgEncOp->intrEnableBit = ((1<<INT_JPU_DONE) | (1<<INT_JPU_ERROR) | (1<<INT_JPU_BIT_BUF_FULL));
    if (pstEncConfig->sliceInterruptEnable)
        pstJpgEncOp->intrEnableBit |= (1<<INT_JPU_SLICE_DONE);
    pstJpgEncOp->streamEndian          = pstEncConfig->StreamEndian;
    pstJpgEncOp->frameEndian           = pstEncConfig->FrameEndian;
    pstJpgEncOp->bitstreamBuffer       = jpgHandle->vbBitstream.phys_addr;
    pstJpgEncOp->bitstreamBufferSize   = jpgHandle->vbBitstream.size;
    pstJpgEncOp->pixelJustification    = pstEncConfig->pixelJustification;
    pstJpgEncOp->rotation              = pstEncConfig->rotation;
    pstJpgEncOp->mirror                = pstEncConfig->mirror;

    JLOG(INFO,"sophgo_jpeg_enc_open_instance bitstreamBuffer:%llx\n", pstJpgEncOp->bitstreamBuffer);

    if(pstJpgEncOp->packedFormat) {
        if (pstJpgEncOp->packedFormat==PACKED_FORMAT_444 && pstJpgEncOp->sourceFormat != FORMAT_444) {
            JLOG(ERR, "Invalid operation mode : In case of using packed mode. sourceFormat must be FORMAT_444\n" );
            goto ERR_ENC_INIT;
        }
    }

    // srcFrameFormat means that it is original source image format.
    srcFrameFormat = pstJpgEncOp->sourceFormat;
    if (pstJpgEncOp->rotation == 90 || pstJpgEncOp->rotation == 270) {
        if (srcFrameFormat == FORMAT_422)
            srcFrameFormat = FORMAT_440;
        else if (srcFrameFormat == FORMAT_440)
            srcFrameFormat = FORMAT_422;
    }
    framebufWidth  = (srcFrameFormat == FORMAT_420 || srcFrameFormat == FORMAT_422) ? JPU_CEIL(16, pstEncConfig->picWidth)  : JPU_CEIL(8, pstEncConfig->picWidth);
    framebufHeight = (srcFrameFormat == FORMAT_420 || srcFrameFormat == FORMAT_440) ? JPU_CEIL(16, pstEncConfig->picHeight) : JPU_CEIL(8, pstEncConfig->picHeight);

    sopCalcSliceHeight(pstJpgEncOp, pstEncConfig->sliceHeight);

    // Open an instance and get initial information for encoding.
    if ((ret=JPU_EncOpen((JpgEncHandle *)&handle, pstJpgEncOp)) != JPG_RET_SUCCESS) {
        JLOG(ERR, "JPU_EncOpen failed Error code is 0x%x \n", ret);
        goto ERR_ENC_INIT;
    }

    jpgHandle->pHandle = handle;
    pJpgInst = (JpgInst *)handle;
    instIdx = pJpgInst->instIndex;
    pEncInfo = &pJpgInst->JpgInfo->encInfo;
    pEncInfo->format = stConfig.eSourceFormat;
    jpgHandle->encHeaderMode = ENC_HEADER_MODE_NORMAL;
    pstEncConfig->encQualityPercentage = stConfig.s32Quality?99:stConfig.s32Quality;

    JPU_EncGiveCommand(handle, SET_JPG_USE_STUFFING_BYTE_FF, &pstEncConfig->bEnStuffByte);
    if (pstEncConfig->encQualityPercentage > 0)  {
        JPU_EncGiveCommand(handle, SET_JPG_QUALITY_FACTOR, &pstEncConfig->encQualityPercentage);
    }

    JPU_EncGiveCommand(handle, GET_JPG_QUALITY_TABLE, &pstEncConfig->encQualityPercentage);

    needFrameBufCount = ENC_SRC_BUF_NUM;
    bitDepth          = (pstJpgEncOp->jpg12bit == FALSE) ? 8 : 12;

    //only support driver test,not for cvi sdk
    if (stConfig.s32SrcType != JPEG_MEM_E_EXTERNAL) {
        // Initialize frame buffers for encoding and source frame
        if (!AllocateFrameBuffer(instIdx, pstJpgEncOp->sourceFormat, pstJpgEncOp->chromaInterleave, pstJpgEncOp->packedFormat,
                0, FALSE, framebufWidth, framebufHeight, bitDepth, needFrameBufCount)) {
            goto ERR_ENC_INIT;
        }

        for( i = 0; i < needFrameBufCount; ++i ) {
            jpgHandle->pFrame[i] = GetFrameBuffer(instIdx, i);

            jpgHandle->frameBuf[i].stride  = jpgHandle->pFrame[i]->strideY;
            jpgHandle->frameBuf[i].strideC = jpgHandle->pFrame[i]->strideC;
            jpgHandle->frameBuf[i].bufY    = jpgHandle->pFrame[i]->vbY.phys_addr;
            jpgHandle->frameBuf[i].bufCb   = jpgHandle->pFrame[i]->vbCb.phys_addr;
            jpgHandle->frameBuf[i].bufCr   = jpgHandle->pFrame[i]->vbCr.phys_addr;
#ifdef SUPPORT_PADDING_UNALIGNED_YUV
            jpgHandle->frameBuf[i].fbLumaHeight   = jpgHandle->pFrame[i]->fbLumaHeight;
            jpgHandle->frameBuf[i].fbChromaHeight   = jpgHandle->pFrame[i]->fbChromaHeight;
#endif
        }
    }
    *pvJpgHandle = jpgHandle;
    jpgHandle->stJpgEncOp = *pstJpgEncOp;
    jpgHandle->iCoreIdx = -1;
    jpgHandle->outputInfo.encodeState = -1;
    jpgHandle->pstEncConfig = pstEncConfig;

    vfree(pstJpgEncOp);
    return ret;
ERR_ENC_INIT:
    vfree(pstEncConfig);
    vfree(pstJpgEncOp);
    return ret;
}


static int sopRecreateNewInstance(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, SOPHGO_S_FRAME_BUF *pstFrame)
{
    SOPHGO_JPEG_ENC_HANDLE_INFO *pjpgHandle = (SOPHGO_JPEG_ENC_HANDLE_INFO *)pvJpgHandle;
    JpgRet ret = JPG_RET_SUCCESS;
    JpgEncOutputInfo   info;
    int                 srcFrameFormat = 0;
    int                 framebufWidth = 0, framebufHeight = 0;
    JpgEncInfo          *pEncInfo = 0;
    int instIdx = 0;
    JpgInst *pJpgInst;

    CHECK_PTR_RET_INT(pvJpgHandle);

    pJpgInst = (JpgEncHandle)pjpgHandle->pHandle;

    // delete old instance
    if(pjpgHandle->pHandle) {
        FreeFrameBuffer(pJpgInst->instIndex);
        if (JPU_EncClose(pjpgHandle->pHandle) == JPG_RET_FRAME_NOT_COMPLETE) {
            JPU_EncGetOutputInfo( pjpgHandle->pHandle, &info);
            JPU_EncClose(pjpgHandle->pHandle);
        }
    }
    pjpgHandle->pHandle = NULL;
    pjpgHandle->pstEncConfig->packedFormat = pstFrame->ePackedFormat;
    pjpgHandle->pstEncConfig->sourceSubsample = pstFrame->eFormat;
    pjpgHandle->pstEncConfig->picWidth = pstFrame->s32Width;
    pjpgHandle->pstEncConfig->picHeight = pstFrame->s32Height;

    if (getJpgEncOpenParamDefault(&pjpgHandle->stJpgEncOp, pjpgHandle->pstEncConfig) == FALSE) {
        JLOG(ERR, "getJpgEncOpenParamDefault falied\n");
        return -1;
    }

    if(pjpgHandle->stJpgEncOp.packedFormat) {
        if (pjpgHandle->stJpgEncOp.packedFormat==PACKED_FORMAT_444 && pjpgHandle->stJpgEncOp.sourceFormat != FORMAT_444) {
            JLOG(ERR, "Invalid operation mode : In case of using packed mode. sourceFormat must be FORMAT_444\n" );
            return -2;
        }
    }

    // srcFrameFormat means that it is original source image format.
    srcFrameFormat = pjpgHandle->stJpgEncOp.sourceFormat;
    if (pjpgHandle->stJpgEncOp.rotation == 90 || pjpgHandle->stJpgEncOp.rotation == 270) {
        if (srcFrameFormat == FORMAT_422)
            srcFrameFormat = FORMAT_440;
        else if (srcFrameFormat == FORMAT_440)
            srcFrameFormat = FORMAT_422;
    }
    framebufWidth  = (srcFrameFormat == FORMAT_420 || srcFrameFormat == FORMAT_422) ?
        JPU_CEIL(16, pjpgHandle->pstEncConfig->picWidth)  : JPU_CEIL(8, pjpgHandle->pstEncConfig->picWidth);
    framebufHeight = (srcFrameFormat == FORMAT_420 || srcFrameFormat == FORMAT_440) ?
        JPU_CEIL(16, pjpgHandle->pstEncConfig->picHeight) : JPU_CEIL(8, pjpgHandle->pstEncConfig->picHeight);

    sopCalcSliceHeight(&pjpgHandle->stJpgEncOp, pjpgHandle->pstEncConfig->sliceHeight);

    // Open an instance and get initial information for encoding.
    if ((ret=JPU_EncOpen((JpgEncHandle *)&pjpgHandle->pHandle, &pjpgHandle->stJpgEncOp)) != JPG_RET_SUCCESS) {
        JLOG(ERR, "JPU_EncOpen failed Error code is 0x%x \n", ret);
        return -3;
    }

    pJpgInst = (JpgInst *)pjpgHandle->pHandle;
    instIdx = pJpgInst->instIndex;
    pEncInfo = &pJpgInst->JpgInfo->encInfo;
    pEncInfo->format = pstFrame->eFormat;
    pjpgHandle->encHeaderMode = ENC_HEADER_MODE_NORMAL;

    JPU_EncGiveCommand(pjpgHandle->pHandle, SET_JPG_USE_STUFFING_BYTE_FF, &pjpgHandle->pstEncConfig->bEnStuffByte);
    if (pjpgHandle->pstEncConfig->encQualityPercentage > 0)  {
        JPU_EncGiveCommand(pjpgHandle->pHandle, SET_JPG_QUALITY_FACTOR, &pjpgHandle->pstEncConfig->encQualityPercentage);
    }

    JLOG(ERR,"recreate the instance for format(%d)suc.\n", pstFrame->eFormat);
    return JPG_RET_SUCCESS;
}

int sophgo_jpeg_enc_close_instance(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle)
{
    JpgRet ret = JPG_RET_SUCCESS;
    SOPHGO_JPEG_ENC_HANDLE_INFO *pjpgHandle = (SOPHGO_JPEG_ENC_HANDLE_INFO *)pvJpgHandle;
    JpgInst *pJpgInst;

    CHECK_PTR_RET_INT(pvJpgHandle);

    /* check handle valid */
    ret = CheckJpgInstValidity((JpgEncHandle)pjpgHandle->pHandle);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(INFO, "Invalid handle at cviJpgEncGetInputDataBuf.\n");
        return ret;
    }
    pJpgInst = (JpgEncHandle)pjpgHandle->pHandle;

    if(pjpgHandle->encHeaderParam.pParaSet) {
        jdi_free_dma_memory(&pjpgHandle->vbHeaderBitstream);
        pjpgHandle->encHeaderParam.pParaSet = NULL;
    }

    if(pjpgHandle->vbBitstream.base)
        jdi_free_dma_memory(&pjpgHandle->vbBitstream);

    if(pjpgHandle->pstEncConfig) {
        vfree(pjpgHandle->pstEncConfig);
        pjpgHandle->pstEncConfig = NULL;
    }

    FreeFrameBuffer(pJpgInst->instIndex);

    if (JPU_EncClose(pjpgHandle->pHandle) == JPG_RET_FRAME_NOT_COMPLETE) {
        JLOG(INFO, "NOT_COMPLETE\n");
    }

    kfree(pjpgHandle);

    return 0;
}

int sophgo_jpeg_enc_send_frame_data(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, SOPHGO_S_FRAME_BUF *pstFrame)
{
    JpgRet ret = JPG_RET_SUCCESS;
    JpgEncParam encParam = { 0 };
    FrameBuffer externalBuf;
    int int_reason = 0;
    SOPHGO_JPEG_ENC_HANDLE_INFO *pjpgHandle = (SOPHGO_JPEG_ENC_HANDLE_INFO *)pvJpgHandle;
    JPEG_BS_DATA_STREAM_INFO *pstBsFullInfo = &pjpgHandle->stBsFullInfo;
    JpgInst *pJpgInst;
    JpgEncInfo *pEncInfo;

    CHECK_PTR_RET_INT(pvJpgHandle);
    CHECK_PTR_RET_INT(pstFrame);

    ret = CheckJpgInstValidity(pjpgHandle->pHandle);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(ERR, "CheckJpgInstValidity fail, return %d\n", ret);
        return ret;
    }

    if(((FrameFormat)pstFrame->eFormat != pjpgHandle->stJpgEncOp.sourceFormat)
        || (pstFrame->s32Width && pstFrame->s32Width != pjpgHandle->stJpgEncOp.picWidth)
        || (pstFrame->s32Height && pstFrame->s32Height != pjpgHandle->stJpgEncOp.picHeight)) {
        ret = sopRecreateNewInstance(pvJpgHandle, pstFrame);
        if (ret != JPG_RET_SUCCESS) {
            JLOG(ERR, "sopRecreateNewInstance fail, return %d\n", ret);
            return ret;
        }
    }

    pJpgInst = (JpgInst *)pjpgHandle->pHandle;
    pEncInfo = &pJpgInst->JpgInfo->encInfo;

    // Write picture header
    if (pjpgHandle->encHeaderMode == ENC_HEADER_MODE_NORMAL) {
        pjpgHandle->encHeaderParam.size = 1000;
        if(!pjpgHandle->encHeaderParam.pParaSet) {
            pjpgHandle->vbHeaderBitstream.size = pjpgHandle->encHeaderParam.size;
            pjpgHandle->vbHeaderBitstream.is_cached = 0;
            if (jdi_allocate_dma_memory(&pjpgHandle->vbHeaderBitstream) < 0) {
                JLOG(ERR, "fail to allocate jpeg header bitstream buffer\n" );
                return JPG_RET_FAILURE;
            }

            if (jdi_invalidate_cache(&pjpgHandle->vbHeaderBitstream) < 0) {
                JLOG(ERR, "fail to invalidate jpeg header bitstream buffer\n");
                return JPG_RET_FAILURE;
            }
            pjpgHandle->encHeaderParam.pParaSet = (BYTE *)pjpgHandle->vbHeaderBitstream.virt_addr;
        }
        pjpgHandle->encHeaderParam.headerMode = ENC_HEADER_MODE_NORMAL;            //Encoder header disable/enable control. Annex:A 1.2.3 item 13
        pjpgHandle->encHeaderParam.quantMode = JPG_TBL_NORMAL; //JPG_TBL_MERGE    // Merge quantization table. Annex:A 1.2.3 item 7
        pjpgHandle->encHeaderParam.huffMode  = JPG_TBL_NORMAL; // JPG_TBL_MERGE    //Merge huffman table. Annex:A 1.2.3 item 6
        pjpgHandle->encHeaderParam.disableAPPMarker = 0;                        //Remove APPn. Annex:A item 11
        pjpgHandle->encHeaderParam.enableSofStuffing = TRUE;                        //Remove zero stuffing bits before 0xFFDA. Annex:A item 16.
        if (pjpgHandle->encHeaderParam.headerMode == ENC_HEADER_MODE_NORMAL) {
            if (pjpgHandle->encHeaderParam.pParaSet) {
                //make picture header
                JPU_EncGiveCommand(pjpgHandle->pHandle, ENC_JPG_GET_HEADER, &pjpgHandle->encHeaderParam); // return exact header size int endHeaderparam.siz;
                JLOG(INFO, "JPU_EncGiveCommand[ENC_JPG_GET_HEADER] header size=%d\n", pjpgHandle->encHeaderParam.size);
            }
        }
    }

    externalBuf.bufY = pstFrame->vbY.phys_addr;
    externalBuf.bufCb = pstFrame->vbCb.phys_addr;
    externalBuf.bufCr = pstFrame->vbCr.phys_addr;
    externalBuf.stride = pstFrame->s32StrideY;
    externalBuf.strideC = pstFrame->s32StrideC;
    externalBuf.format = pstFrame->eFormat;
    encParam.sourceFrame = &externalBuf;

    if(pjpgHandle->iCoreIdx < 0) {
        pjpgHandle->iCoreIdx = JpgResRequestOneCore(JPU_INTERRUPT_TIMEOUT_MS);
        if(pjpgHandle->iCoreIdx < 0) {
            JLOG(ERR,"no valid jpu core.\n");
            return JPG_RET_FAILURE;
        }
        JpgEnterLockEx();
        pJpgInst->coreIndex = pjpgHandle->iCoreIdx;
        JpgResSetTaskCore(task_pid_nr(current), pjpgHandle->iCoreIdx);
        JpgLeaveLockEx();
        JLOG(INFO,"sophgo_jpeg_enc_send_frame_data core:%d\n", pjpgHandle->iCoreIdx);
    }

    // Set jpeg extension address for framebuffer.
    JpgEnterLockEx();
    JPU_SetExtAddr(pjpgHandle->iCoreIdx, externalBuf.bufY >> 32);
    JpgLeaveLockEx();

    // Start encoding a frame.
    ret = JPU_EncStartOneFrame(pjpgHandle->pHandle, &encParam);
    if( ret != JPG_RET_SUCCESS ) {
        JLOG(ERR, "JPU_EncStartOneFrame failed Error code is 0x%x \n", ret );
        goto JPG_ENC_ENCODE_END;
    }
    pjpgHandle->iCoreIdx = pJpgInst->coreIndex;

    while(1) {
        int_reason = JPU_WaitInterrupt(pjpgHandle->pHandle, JPU_INTERRUPT_TIMEOUT_MS);
        if (int_reason == -1) {
            int i;
            JLOG(ERR, "Error enc: timeout happened,core:%d inst %d, reason:%d\n",
                pjpgHandle->iCoreIdx, pJpgInst->instIndex, int_reason);
            // JPU_SWReset(pJpgInst->coreIndex, pjpgHandle->pHandle);
            ret = JPG_RET_FAILURE;
            for (i=(pJpgInst->instIndex*NPT_REG_SIZE); i<=((pJpgInst->instIndex*NPT_REG_SIZE)+0x250); i=i+16)
            {
                JLOG(ERR, "0x%04xh: 0x%08lx 0x%08lx 0x%08lx 0x%08lx\n", i,
                    jdi_read_register_ext(pjpgHandle->iCoreIdx, i),
                    jdi_read_register_ext(pjpgHandle->iCoreIdx, i+4),
                    jdi_read_register_ext(pjpgHandle->iCoreIdx, i+8),
                    jdi_read_register_ext(pjpgHandle->iCoreIdx, i+0xc));
            }
            JPU_SetJpgPendingInstEx(pjpgHandle->pHandle, NULL);
            goto JPG_ENC_ENCODE_END;
        }
        if (int_reason == -2) {
            JLOG(ERR, "Interrupt occurred. but this interrupt is not for my instance enc\n");
            continue;
        }

        if (int_reason & (1<<INT_JPU_DONE) || int_reason & (1<<INT_JPU_SLICE_DONE)) {  // Must catch PIC_DONE interrupt before catching EMPTY interrupt
            // Do no clear INT_JPU_DONE these will be cleared in JPU_EncGetOutputInfo.
            break;
        }

        if (int_reason & (1<<INT_JPU_BIT_BUF_FULL)) {
            JLOG(WARN, "INT_JPU_BIT_BUF_FULL interrupt issued INSTANCE %d \n", pJpgInst->instIndex);
            //FIXME need to save the pre data
            sopProcessBsFull(pvJpgHandle);
            JPU_EncUpdateBitstreamBuffer(pjpgHandle->pHandle, 0);
            JPU_ClrStatus(pjpgHandle->pHandle, (1<<INT_JPU_BIT_BUF_FULL));
            continue;
        }
    }

    if ((ret=JPU_EncGetOutputInfo(pjpgHandle->pHandle, &pjpgHandle->outputInfo)) != JPG_RET_SUCCESS) {
        JLOG(ERR, "JPU_EncGetOutputInfo failed Error code is 0x%x \n", ret );
        goto JPG_ENC_ENCODE_END;
    }

    if(pstBsFullInfo->length) {
        memcpy((void*)(pstBsFullInfo->vbBitstreamExt.virt_addr + pstBsFullInfo->length),
            (void *)pjpgHandle->vbBitstream.virt_addr, pjpgHandle->outputInfo.bitstreamSize);
        pstBsFullInfo->length += pjpgHandle->outputInfo.bitstreamSize;
    }
    JPU_EncUpdateBitstreamBuffer(pjpgHandle->pHandle, 0);
    pjpgHandle->i32ValidCnt++;

JPG_ENC_ENCODE_END:
    if(pjpgHandle->iCoreIdx >= 0) {
        JLOG(INFO,"sophgo_jpeg_enc_send_frame_data end core:%d\n", pjpgHandle->iCoreIdx);
        JpgEnterLockEx();
        JpgResReleaseCore(pjpgHandle->iCoreIdx);
        pJpgInst->coreIndex = -1;
        pjpgHandle->iCoreIdx = -1;
        JpgLeaveLockEx();
    }

    return ret;
}

int sophgo_jpeg_enc_get_stream(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, void *pvData)
{
    JpgRet ret = JPG_RET_SUCCESS;
    SOPHGO_JPEG_ENC_HANDLE_INFO *pjpgHandle = (SOPHGO_JPEG_ENC_HANDLE_INFO *)pvJpgHandle;
    JPEG_BS_DATA_STREAM_INFO *pstBsFullInfo = &pjpgHandle->stBsFullInfo;
    SOPHGO_S_BUF *pStream = (SOPHGO_S_BUF *)pvData;

    CHECK_PTR_RET_INT(pvJpgHandle);
    CHECK_PTR_RET_INT(pvData);

    ret = CheckJpgInstValidity(pjpgHandle->pHandle);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(INFO, "CheckJpgInstValidity fail, return %d\n", ret);
        return ret;
    }

    if(pjpgHandle->outputInfo.encodeState != ENCODE_STATE_FRAME_DONE) {
        JLOG(INFO, "sophgo_jpeg_enc_get_stream not ready, return %d\n", ret);
        return ret;
    }
    if(pjpgHandle->i32ValidCnt > 0)
        pjpgHandle->i32ValidCnt--;

    if(pjpgHandle->vbHeaderBitstream.phys_addr && pjpgHandle->encHeaderParam.size) {
        pStream->phys_addr = pjpgHandle->vbHeaderBitstream.phys_addr;
        pStream->virt_addr = pjpgHandle->vbHeaderBitstream.virt_addr;
        pStream->u32Size = pjpgHandle->encHeaderParam.size;
        JLOG(INFO,"sophgo_jpeg_enc_get_stream header size:%ld\n", pStream->u32Size);
        pStream = pStream+1;
    }
    if(!pstBsFullInfo->length) {
        pStream->phys_addr = pjpgHandle->vbBitstream.phys_addr;
        pStream->virt_addr = pjpgHandle->vbBitstream.virt_addr;
        pStream->u32Size = pjpgHandle->outputInfo.bitstreamSize;
    } else {
        pStream->phys_addr = pstBsFullInfo->vbBitstreamExt.phys_addr;
        pStream->virt_addr = pstBsFullInfo->vbBitstreamExt.virt_addr;
        pStream->u32Size = pstBsFullInfo->length;
    }

    JLOG(INFO,"sophgo_jpeg_enc_get_stream body addr:%lx size:%d len:%ld\n",
        pStream->phys_addr, pjpgHandle->outputInfo.bitstreamSize, pstBsFullInfo->length);
    //pjpgHandle->outputInfo.encodeState = -1;

    //if cached ,need to invalid the cache here

    return ret;
}

int sophgo_jpeg_enc_get_input_source_data(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, SOPHGO_S_FRAME_BUF *pvData)
{
    JpgRet ret = JPG_RET_SUCCESS;
    SOPHGO_JPEG_ENC_HANDLE_INFO *pjpgHandle = (SOPHGO_JPEG_ENC_HANDLE_INFO *)pvJpgHandle;
    JpgInst *pJpgInst;
    JpgEncInfo *pEncInfo;
    SOPHGO_S_FRAME_BUF *pstFrameBuf;

    CHECK_PTR_RET_INT(pvJpgHandle);
    CHECK_PTR_RET_INT(pvData);

    /* check handle valid */
    ret = CheckJpgInstValidity((JpgEncHandle)pjpgHandle->pHandle);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(INFO, "Invalid handle at cviJpgEncGetInputDataBuf.\n");
        return ret;
    }
    pJpgInst = (JpgEncHandle)pjpgHandle->pHandle;
    pEncInfo = &pJpgInst->JpgInfo->encInfo;

    if (NULL == pjpgHandle->pFrame)
        return JPG_RET_WRONG_CALL_SEQUENCE;
    if (NULL == pvData)
        return JPG_RET_INVALID_PARAM;

    pstFrameBuf = (SOPHGO_S_FRAME_BUF *)pvData;
    pstFrameBuf->eFormat = pEncInfo->format;
    memcpy(&(pstFrameBuf->vbY), &(pjpgHandle->pFrame[0]->vbY),
           sizeof(jpu_buffer_t));
    memcpy(&(pstFrameBuf->vbCb), &(pjpgHandle->pFrame[0]->vbCb),
           sizeof(jpu_buffer_t));
    memcpy(&(pstFrameBuf->vbCr), &(pjpgHandle->pFrame[0]->vbCr),
           sizeof(jpu_buffer_t));
    pstFrameBuf->s32StrideY = pjpgHandle->pFrame[0]->strideY;
    pstFrameBuf->s32StrideC = pjpgHandle->pFrame[0]->strideC;
    JLOG(INFO,"sophgo_jpeg_enc_get_input_source_data vbY.p:%lx vbCb.p:%lx vbCr.p:%lx\n",
        pstFrameBuf->vbY.phys_addr, pstFrameBuf->vbCb.phys_addr, pstFrameBuf->vbCr.phys_addr);
    JLOG(INFO,"sophgo_jpeg_enc_get_input_source_data vbY.v:%lx vbCb.v:%lx vbCr.v:%lx\n",
        pstFrameBuf->vbY.virt_addr, pstFrameBuf->vbCb.virt_addr, pstFrameBuf->vbCr.virt_addr);

    if (pEncInfo->chromaInterleave)
        pstFrameBuf->s32StrideC *= 2;

    return JPG_RET_SUCCESS;
}


int sophgo_jpeg_enc_release_frame(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle)
{
    JpgRet ret = JPG_RET_SUCCESS;
    SOPHGO_JPEG_ENC_HANDLE_INFO *pjpgHandle = (SOPHGO_JPEG_ENC_HANDLE_INFO *)pvJpgHandle;
    JPEG_BS_DATA_STREAM_INFO *pstBsFullInfo = &pjpgHandle->stBsFullInfo;

    CHECK_PTR_RET_INT(pvJpgHandle);

    ret = CheckJpgInstValidity(pjpgHandle->pHandle);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(ERR, "CheckJpgInstValidity fail, return %d\n", ret);
        return ret;
    }
    if(pstBsFullInfo->length && pstBsFullInfo->vbBitstreamExt.base) {
        jdi_free_dma_memory(&pstBsFullInfo->vbBitstreamExt);
    }
    pstBsFullInfo->length = 0;

    return ret;
}

int sophgo_jpeg_enc_get_valid_bitstream_count(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle)
{
    JpgRet ret = JPG_RET_SUCCESS;
    SOPHGO_JPEG_ENC_HANDLE_INFO *pjpgHandle = (SOPHGO_JPEG_ENC_HANDLE_INFO *)pvJpgHandle;

    CHECK_PTR_RET_INT(pvJpgHandle);

    ret = CheckJpgInstValidity(pjpgHandle->pHandle);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(ERR, "CheckJpgInstValidity fail, return %d\n", ret);
        return 0;
    }

    return pjpgHandle->i32ValidCnt;
}
