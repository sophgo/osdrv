#include "sophgo_jpeg_internal.h"
#include "sophgo_jpeg_interface.h"
#include "jputypes.h"
#include "jdi.h"


typedef struct {
    unsigned char *buf;
    int size;
    int point;
    int count;
    int fillendbs;
} BufInfo;


typedef struct {
    JpgDecHandle        pHandle;
    jpu_buffer_t        vbBitstream;
    FrameBuffer         frameBuf[MAX_FRAME_JPU];
    FRAME_BUF           *pFrame[MAX_FRAME_JPU];
    Uint32              outNum;
    Uint32              iHorScaleMode;
    Uint32              iVerScaleMode;
    Int32               iCoreIdx;
    JpgDecOpenParam     decOP;
    JpgDecOutputInfo    outputInfo;
    int                 i32ValidCnt;
    unsigned char       bExternalMem;
}SOPHGO_JPEG_DEC_HANDLE_INFO,*SOPHGO_JPEG_DEC_HANDLE_INFO_PTR;

/* DEC */
int sophgo_jpeg_dec_open_instance(SOPHGO_JPEG_HANDLE_PTR *pvJpgHandle, SOPHGO_S_DEC_CONFIG_PARAM stConfig)
{
    JpgRet ret = JPG_RET_SUCCESS;
    JpgDecOpenParam     decOP        = {0};
    SOPHGO_JPEG_DEC_HANDLE_INFO_PTR jpgHandle;

    jpgHandle = kzalloc(sizeof(SOPHGO_JPEG_DEC_HANDLE_INFO), GFP_KERNEL);
    if(!jpgHandle)
        return JPG_RET_FAILURE;

    // Open an instance and get initial information for decoding.
    jpgHandle->vbBitstream.size = (stConfig.bsSize == 0) ? STREAM_BUF_SIZE : stConfig.bsSize;
    if(jpgHandle->vbBitstream.size < 4096)
        jpgHandle->vbBitstream.size = 4096;
    jpgHandle->vbBitstream.size = (jpgHandle->vbBitstream.size + 1023) & ~1023; // ceil128(size)
    jpgHandle->vbBitstream.is_cached = 0;
    if (jdi_allocate_dma_memory(&jpgHandle->vbBitstream) < 0) {
        JLOG(ERR, "fail to allocate bitstream buffer size:%ld\n", jpgHandle->vbBitstream.size);
        goto ERR_DEC_JPU_OPEN;
    }

    if (jdi_invalidate_cache(&jpgHandle->vbBitstream) < 0) {
        JLOG(ERR, "fail to invalidate bitstream buffer cache addr:%lx, size:%ld\n", jpgHandle->vbBitstream.phys_addr, jpgHandle->vbBitstream.size);
        goto ERR_DEC_JPU_OPEN;
    }

    jpgHandle->outNum           = 1;
    jpgHandle->outputInfo.indexFrameDisplay = -1;
    jpgHandle->iCoreIdx         = -1;
    jpgHandle->iHorScaleMode    = stConfig.iHorScaleMode;
    jpgHandle->iVerScaleMode    = stConfig.iVerScaleMode;
    decOP.streamEndian          = stConfig.StreamEndian;
    decOP.frameEndian           = stConfig.FrameEndian;
    decOP.bitstreamBuffer       = jpgHandle->vbBitstream.phys_addr;
    decOP.bitstreamBufferSize   = jpgHandle->vbBitstream.size;
    //set virtual address mapped of physical address
    decOP.pBitStream            = (BYTE *)jpgHandle->vbBitstream.virt_addr; //lint !e511
    decOP.chromaInterleave      = stConfig.stDecBuf.eCbcrInterleave;
    decOP.packedFormat          = stConfig.stDecBuf.ePackedFormat;
    decOP.roiEnable             = stConfig.roiEnable;
    decOP.roiOffsetX            = stConfig.roiOffsetX;
    decOP.roiOffsetY            = stConfig.roiOffsetY;
    decOP.roiWidth              = stConfig.roiWidth;
    decOP.roiHeight             = stConfig.roiHeight;
    decOP.rotation              = stConfig.rotation;
    decOP.mirror                = stConfig.mirror;
    decOP.pixelJustification    = stConfig.pixelJustification;
    decOP.outputFormat          = stConfig.stDecBuf.eFormat;//FORMAT_MAX;
    decOP.intrEnableBit         = ((1<<INT_JPU_DONE) | (1<<INT_JPU_ERROR) | (1<<INT_JPU_BIT_BUF_EMPTY));

    JLOG(INFO,"sophgo_jpeg_dec_open_instance bitstream %llx size:%d\n",decOP.bitstreamBuffer, decOP.bitstreamBufferSize);

    ret = JPU_DecOpen(&jpgHandle->pHandle, &decOP);
    if( ret != JPG_RET_SUCCESS ) {
        JLOG(ERR, "JPU_DecOpen failed Error code is 0x%x \n", ret );
        goto ERR_DEC_JPU_OPEN;
    }
    jpgHandle->decOP = decOP;
    *pvJpgHandle = jpgHandle;

    return ret;
ERR_DEC_JPU_OPEN:
    if (jpgHandle->vbBitstream.phys_addr) {
        jdi_free_dma_memory(&jpgHandle->vbBitstream);
    }
    kfree(jpgHandle);

    return ret;
}

int sophgo_jpeg_dec_close_instance(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle)
{
    JpgRet ret = JPG_RET_SUCCESS;
    SOPHGO_JPEG_DEC_HANDLE_INFO *pjpgHandle = (SOPHGO_JPEG_DEC_HANDLE_INFO *)pvJpgHandle;

    CHECK_PTR_RET_INT(pvJpgHandle);

    /* check handle valid */
    ret = CheckJpgInstValidity((JpgDecHandle)pjpgHandle->pHandle);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(ERR, "CheckJpgInstValidity fail, return %d\n", ret);
        return ret;
    }

    if(pjpgHandle->vbBitstream.base)
        jdi_free_dma_memory(&pjpgHandle->vbBitstream);

    // Now that we are done with decoding, close the open instance.
	if (JPU_DecClose(pjpgHandle->pHandle) == JPG_RET_FRAME_NOT_COMPLETE) {
		JLOG(INFO, "NOT_COMPLETE\n");
	}

    kfree(pjpgHandle);

    return ret;
}


static int FillSdramBurst(BufInfo *pBufInfo, PhysicalAddress targetAddr,
           PhysicalAddress bsBufStartAddr, PhysicalAddress bsBufEndAddr,
           Uint32 size, int checkeos, int *streameos, int endian)
{
    Uint8 *pBuf;
    int room;

    pBufInfo->count = 0;

    if (checkeos == 1 && (pBufInfo->point >= pBufInfo->size)) {
        *streameos = 1;
        return 0;
    }

    if ((pBufInfo->size - pBufInfo->point) < (int)size)
        pBufInfo->count = (pBufInfo->size - pBufInfo->point);
    else
        pBufInfo->count = size;

    pBuf = pBufInfo->buf + pBufInfo->point;
    if ((targetAddr + pBufInfo->count) > bsBufEndAddr) {
        room = bsBufEndAddr - targetAddr;
        JpuWriteMem(targetAddr, pBuf, room, endian);
        // flush here if cached
        JpuWriteMem(bsBufStartAddr, pBuf + room,
                (pBufInfo->count - room), endian);
        // flush here if cached
    } else {
        JpuWriteMem(targetAddr, pBuf, pBufInfo->count, endian);
        // flush here if cached
    }

    pBufInfo->point += pBufInfo->count;
    return pBufInfo->count;
}


static JpgRet WriteJpgBsBufHelper(JpgDecHandle handle, BufInfo *pBufInfo,
               PhysicalAddress paBsBufStart,
               PhysicalAddress paBsBufEnd, int defaultsize,
               int checkeos, int *pstreameos, int endian)
{
    JpgRet ret = JPG_RET_SUCCESS;
    int size = 0;
    int fillSize = 0;
    PhysicalAddress paRdPtr, paWrPtr;

    ret = JPU_DecGetBitstreamBuffer(handle, &paRdPtr, &paWrPtr, &size);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(ERR,
             "JPU_DecGetBitstreamBuffer failed Error code is 0x%x \n",
             ret);
        goto FILL_BS_ERROR;
    }

    if (size <= 0) {
        return JPG_RET_INSUFFICIENT_BITSTREAM_BUFFER;
    }

    if (defaultsize) {
        if (size < defaultsize)
            fillSize = size;
        else
            fillSize = defaultsize;
    } else {
        fillSize = size;
    }

    fillSize = FillSdramBurst(pBufInfo, paWrPtr, paBsBufStart, paBsBufEnd,
                  fillSize, checkeos, pstreameos, endian);

    if (*pstreameos == 0) {
        ret = JPU_DecUpdateBitstreamBuffer(handle, fillSize);
        if (ret != JPG_RET_SUCCESS) {
            JLOG(ERR,
                 "JPU_DecUpdateBitstreamBuffer failed Error code is 0x%x \n",
                 ret);
            goto FILL_BS_ERROR;
        }

        if ((pBufInfo->size - pBufInfo->point) <= 0) {
            ret = JPU_DecUpdateBitstreamBuffer(handle,
                               STREAM_END_SIZE);
            if (ret != JPG_RET_SUCCESS) {
                JLOG(ERR,
                     "JPU_DecUpdateBitstreamBuffer failed Error code is 0x%x \n",
                     ret);
                goto FILL_BS_ERROR;
            }

            pBufInfo->fillendbs = 1;
        }
    } else {
        if (!pBufInfo->fillendbs) {
            ret = JPU_DecUpdateBitstreamBuffer(handle,
                               STREAM_END_SIZE);
            if (ret != JPG_RET_SUCCESS) {
                JLOG(ERR,
                     "JPU_DecUpdateBitstreamBuffer failed Error code is 0x%x \n",
                     ret);
                goto FILL_BS_ERROR;
            }
            pBufInfo->fillendbs = 1;
        }
    }

FILL_BS_ERROR:

    return ret;
}

static int _get_jpeg_info_and_register_framebuffer(SOPHGO_JPEG_DEC_HANDLE_INFO *pjpgHandle)
{
    JpgRet ret = JPG_RET_SUCCESS;
    Uint32 needFrameBufCount = 1;
    Uint32 framebufWidth = 0;
    Uint32 framebufHeight = 0;
    Uint32 framebufStride = 0;
    JpgDecInitialInfo initialInfo = { 0 };
    JpgInst *pJpgInst;
    JpgDecInfo *pDecInfo;
    Uint32              decodingWidth, decodingHeight;
    Uint32              displayWidth, displayHeight;
    FrameFormat         subsample;
    Uint32              bitDepth = 0;
    Uint32              temp;
    BOOL                scalerOn = FALSE;
    JpgDecHandle handle;
    Int32 instIdx;
    Uint32 lumaStride, chromaStride, lumaHeight, chromaHeight;
    Int32 i;

    CHECK_PTR_RET_INT(pjpgHandle);
    pJpgInst = (JpgInst *)pjpgHandle->pHandle;
    handle = (JpgDecHandle)pjpgHandle->pHandle;
    pDecInfo = &pJpgInst->JpgInfo->decInfo;
    instIdx = pJpgInst->instIndex;

    /* LOG HEADER */
    JLOG(INFO, "I   F    FB_INDEX  FRAME_START  ECS_START  CONSUME   RD_PTR   WR_PTR      CYCLE\n");
    JLOG(INFO, "-------------------------------------------------------------------------------\n");

    if ((ret=JPU_DecGetInitialInfo(handle, &initialInfo)) != JPG_RET_SUCCESS) {
        if (JPG_RET_BIT_EMPTY == ret) {
            JLOG(INFO, "<%s:%d> BITSTREAM EMPTY\n", __FUNCTION__, __LINE__);
            return ret;
        }
        else {
            JLOG(ERR, "JPU_DecGetInitialInfo failed Error code is 0x%x, inst=%d \n", ret, instIdx);
            return ret;
        }
    }

    JLOG(INFO,"_get_jpeg_info_and_register_framebuffer JPU_DecGetInitialInfo end.\n");
    if (initialInfo.sourceFormat == FORMAT_420 || initialInfo.sourceFormat == FORMAT_422)
        framebufWidth = JPU_CEIL(16, initialInfo.picWidth);
    else
        framebufWidth  = JPU_CEIL(8, initialInfo.picWidth);

    if (initialInfo.sourceFormat == FORMAT_420 || initialInfo.sourceFormat == FORMAT_440)
        framebufHeight = JPU_CEIL(16, initialInfo.picHeight);
    else
        framebufHeight = JPU_CEIL(8, initialInfo.picHeight);

    decodingWidth  = framebufWidth  >> pDecInfo->iHorScaleMode;
    decodingHeight = framebufHeight >> pDecInfo->iVerScaleMode;
    if (pDecInfo->packedFormat != PACKED_FORMAT_NONE && pDecInfo->packedFormat != PACKED_FORMAT_444) {
        // When packed format, scale-down resolution should be multiple of 2.
        decodingWidth  = JPU_CEIL(2, decodingWidth);
    }

    subsample = (pjpgHandle->decOP.outputFormat == FORMAT_MAX) ? initialInfo.sourceFormat : pjpgHandle->decOP.outputFormat;

    temp           = decodingWidth;
    decodingWidth  = (pDecInfo->rotationIndex == 1 || pDecInfo->rotationIndex == 3) ? decodingHeight : decodingWidth;
    decodingHeight = (pDecInfo->rotationIndex == 1 || pDecInfo->rotationIndex == 3) ? temp           : decodingHeight;
    if(pDecInfo->roiEnable == TRUE) {
        decodingWidth  = framebufWidth  = initialInfo.roiFrameWidth ;
        decodingHeight = framebufHeight = initialInfo.roiFrameHeight;
    }

    if (0 != pDecInfo->iHorScaleMode || 0 != pDecInfo->iVerScaleMode) {
        displayWidth  = JPU_FLOOR(2, (framebufWidth >> pDecInfo->iHorScaleMode));
        displayHeight = JPU_FLOOR(2, (framebufHeight >> pDecInfo->iVerScaleMode));
    }
    else {
        displayWidth  = decodingWidth;
        displayHeight = decodingHeight;
    }
    JLOG(INFO, "decodingWidth: %d, decodingHeight: %d\n", decodingWidth, decodingHeight);

    // Check restrictions
    if (pDecInfo->rotationIndex != 0 || pDecInfo->mirrorIndex != MIRDIR_NONE) {
        if (pjpgHandle->decOP.outputFormat != FORMAT_MAX
            && pjpgHandle->decOP.outputFormat != initialInfo.sourceFormat) {
            JLOG(ERR, "The rotator cannot work with the format converter together.\n");
            return JPG_RET_FAILURE;
        }
    }


    JLOG(INFO, "<INSTANCE %d>\n", instIdx);
    JLOG(INFO, "SOURCE PICTURE SIZE : W(%d) H(%d)\n", initialInfo.picWidth, initialInfo.picHeight);
    JLOG(INFO, "DECODED PICTURE SIZE: W(%d) H(%d)\n", displayWidth, displayHeight);
    JLOG(INFO, "SUBSAMPLE           : %d\n",          subsample);

    JLOG(INFO, "[PF] DECODED PICTURE SIZE: W(%d) H(%d)\n", displayWidth, displayHeight);
    JLOG(INFO, "[PF] SUBSAMPLE           : %d\n",          subsample);

    //Allocate frame buffer
    needFrameBufCount = initialInfo.minFrameBufferCount;
    bitDepth          = initialInfo.bitDepth;

    scalerOn          = (BOOL)(pDecInfo->iHorScaleMode || pDecInfo->iVerScaleMode);
    if (pjpgHandle->bExternalMem) {
        GetFrameBufStride(subsample, 0, pDecInfo->packedFormat, scalerOn, decodingWidth,
        decodingHeight, bitDepth, &lumaStride, &lumaHeight, &chromaStride, &chromaHeight);

        pjpgHandle->frameBuf[0].stride  = lumaStride;
        pjpgHandle->frameBuf[0].strideC = chromaStride;
        pjpgHandle->frameBuf[0].endian  = pDecInfo->frameEndian;
        pjpgHandle->frameBuf[0].format  = subsample;
        pjpgHandle->frameBuf[0].fbLumaHeight = lumaHeight;
        pjpgHandle->frameBuf[0].fbChromaHeight = chromaHeight;
        needFrameBufCount = 1;
    } else {
        if (AllocateFrameBuffer(instIdx, subsample, pjpgHandle->decOP.chromaInterleave , pDecInfo->packedFormat,
            pDecInfo->rotationIndex * 90, scalerOn, decodingWidth, decodingHeight, bitDepth, needFrameBufCount) == FALSE) {
            JLOG(ERR, "Failed to AllocateFrameBuffer()\n");
            return JPG_RET_FAILURE;
        }

        for( i = 0; i < needFrameBufCount; ++i ) {
            pjpgHandle->pFrame[i] = GetFrameBuffer(instIdx, i);
            pjpgHandle->frameBuf[i].bufY    = pjpgHandle->pFrame[i]->vbY.phys_addr;
            pjpgHandle->frameBuf[i].bufCb = pjpgHandle->pFrame[i]->vbCb.phys_addr;
            if (pDecInfo->chromaInterleave == CBCR_SEPARATED)
                pjpgHandle->frameBuf[i].bufCr = pjpgHandle->pFrame[i]->vbCr.phys_addr;
            pjpgHandle->frameBuf[i].stride  = pjpgHandle->pFrame[i]->strideY;
            pjpgHandle->frameBuf[i].strideC = pjpgHandle->pFrame[i]->strideC;
            pjpgHandle->frameBuf[i].endian  = pDecInfo->frameEndian;
            pjpgHandle->frameBuf[i].format  = (FrameFormat)pjpgHandle->pFrame[i]->Format;
        }
    }
    framebufStride = pjpgHandle->frameBuf[0].stride;

    // Register frame buffers requested by the decoder.
    if ((ret=JPU_DecRegisterFrameBuffer(handle, pjpgHandle->frameBuf, needFrameBufCount, framebufStride)) != JPG_RET_SUCCESS) {
        JLOG(ERR, "JPU_DecRegisterFrameBuffer failed Error code is 0x%x \n", ret );
        return ret;
    }

    if ((ret = JPU_DecGiveCommand(handle, SET_JPG_SCALE_HOR,  &(pjpgHandle->iHorScaleMode))) != JPG_RET_SUCCESS) {
         JLOG(ERR, "JPU_DecGiveCommand[SET_JPG_SCALE_HOR] failed Error code is 0x%x \n", ret );
         return ret;
     }
     if ((ret = JPU_DecGiveCommand(handle, SET_JPG_SCALE_VER,  &(pjpgHandle->iVerScaleMode))) != JPG_RET_SUCCESS) {
         JLOG(ERR, "JPU_DecGiveCommand[SET_JPG_SCALE_VER] failed Error code is 0x%x \n", ret );
         return ret;
     }

    return ret;
}

static unsigned int jpg_dump_cnt=0;
void dump_bitstream_file(int core_idx,int idx, unsigned char *buf, unsigned int length)
{
    char writeName[256];
    drv_file_t *fp;

    sprintf(writeName, "/tmp/bitstream_%d_%d_%d.jpeg", core_idx, idx, jpg_dump_cnt++);
    fp = drv_fopen(writeName, "wb");
    if (0 == fp) {
        JLOG(ERR, "Cann't create a file to write data\n");
        return;
    }

    drv_fwrite(buf,sizeof(unsigned char), length, fp);
    drv_fclose(fp);
    return;
}

int sophgo_jpeg_dec_send_frame_data(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, void *pvData, int s32Length)
{
    JpgRet ret = JPG_RET_SUCCESS;
    JpgDecParam decParam = { 0 };
    int int_reason = 0;
    int streameos = 0;
    BufInfo bufInfo = { 0 };
    JpgInst *pJpgInst;
    JpgDecInfo *pDecInfo;
    JpgDecHandle handle;
    int coreIdx = -1;
    SOPHGO_JPEG_DEC_HANDLE_INFO *pjpgHandle = (SOPHGO_JPEG_DEC_HANDLE_INFO *)pvJpgHandle;

    CHECK_PTR_RET_INT(pjpgHandle);
    CHECK_PTR_RET_INT(pvData);

    pJpgInst = (JpgInst *)pjpgHandle->pHandle;
    handle = (JpgDecHandle)pjpgHandle->pHandle;

    /* check handle valid */
    ret = CheckJpgInstValidity(pjpgHandle->pHandle);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(INFO, "CheckJpgInstValidity fail, return %d\n", ret);
        return ret;
    }

    pDecInfo = &pJpgInst->JpgInfo->decInfo;
    //FIXME we should get a core ip here
    // write the jpeg data to jpu
    bufInfo.buf = pvData;
    bufInfo.size = s32Length;
    bufInfo.point = 0;

    //dump_bitstream_file(pJpgInst->instIndex, bufInfo.buf, bufInfo.size);

    if(pjpgHandle->iCoreIdx < 0) {
        coreIdx = JpgResRequestOneCore(JPU_INTERRUPT_TIMEOUT_MS);
        if(coreIdx < 0) {
            JLOG(ERR,"no valid jpu core.\n");
            return JPG_RET_FAILURE;
        }

        JpgEnterLockEx();
        pJpgInst->coreIndex = coreIdx;
        pjpgHandle->iCoreIdx = coreIdx;
        JpgResSetTaskCore(task_pid_nr(current), coreIdx);
        JpgLeaveLockEx();
        JLOG(INFO,"sophgo_jpeg_dec_send_frame_data core:%d\n", pjpgHandle->iCoreIdx);
    }
    ret = WriteJpgBsBufHelper((JpgDecHandle) pjpgHandle->pHandle, &bufInfo,
                      pDecInfo->streamBufStartAddr,
                      pDecInfo->streamBufEndAddr, 0, 0, &streameos,
                      pDecInfo->streamEndian);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(ERR, "WriteBsBufHelper failed Error code is 0x%x \n", ret);
        goto SEND_DEC_STREAM_ERR;
    }

    if (1 != pDecInfo->initialInfoObtained) {
        ret = _get_jpeg_info_and_register_framebuffer(pjpgHandle);
        if(ret != JPG_RET_SUCCESS) {
            goto SEND_DEC_STREAM_ERR;
        }
    }

    // Set jpeg extension address for framebuffer.
    JpgEnterLockEx();
    JPU_SetExtAddr(pjpgHandle->iCoreIdx, pjpgHandle->frameBuf[0].bufY >> 32);
    JpgLeaveLockEx();

    // Start decoding a frame.
    ret = JPU_DecStartOneFrame(handle, &decParam);
    if (ret != JPG_RET_SUCCESS && ret != JPG_RET_EOS) {
        if (ret == JPG_RET_BIT_EMPTY) {
            JLOG(ERR, "BITSTREAM NOT ENOUGH.............\n");
            goto SEND_DEC_STREAM_ERR;
        }

        JLOG(ERR, "JPU_DecStartOneFrame failed Error code is 0x%x \n", ret );
        goto SEND_DEC_STREAM_ERR;
    }
    JLOG(INFO, "JPU_DecStartOneFrame ret code is 0x%x \n", ret );

    //JLOG(INFO, "\t<+>INSTANCE #%d JPU_WaitInterrupt\n", handle->instIndex);
    while(1) {
        if ((int_reason=JPU_WaitInterrupt(handle, JPU_INTERRUPT_TIMEOUT_MS)) == -1) {
            JLOG(ERR, "Error dec: timeout happened,core:%d inst %d, reason:%d\n",
                pjpgHandle->iCoreIdx, pJpgInst->instIndex, int_reason);
            // dump_bitstream_file(pjpgHandle->iCoreIdx, pJpgInst->instIndex, bufInfo.buf, bufInfo.size);
            // JPU_SWReset(pJpgInst->coreIndex, handle);
            JPU_SetJpgPendingInstEx(handle, NULL);
            ret = JPG_RET_FAILURE;
            goto SEND_DEC_STREAM_ERR;
        }
        if (int_reason == -2) {
            JLOG(ERR, "Interrupt occurred. but this interrupt is not for my instance dec\n");
            continue;
        }

        if (int_reason & ((1<<INT_JPU_DONE) | (1<<INT_JPU_ERROR) | (1<<INT_JPU_SLICE_DONE))) {
            // Do no clear INT_JPU_DONE and INT_JPU_ERROR interrupt. these will be cleared in JPU_DecGetOutputInfo.
            JLOG(INFO, "\tINSTANCE #%d int_reason: %08x\n", handle->instIndex, int_reason);
            ret = JPG_RET_SUCCESS;
            break;
        }

        if (int_reason & (1<<INT_JPU_BIT_BUF_EMPTY)) {
            //need more jpeg data,we need the same coreidx here?
            JPU_ClrStatus(handle, (1<<INT_JPU_BIT_BUF_EMPTY));
            ret = JPG_RET_BIT_EMPTY;
            break;
        }
    }

    if ((ret=JPU_DecGetOutputInfo(handle, &pjpgHandle->outputInfo)) != JPG_RET_SUCCESS) {
        JLOG(ERR, "JPU_DecGetOutputInfo failed Error code is 0x%x \n", ret );
        goto SEND_DEC_STREAM_ERR;
    }

    if (pjpgHandle->outputInfo.decodingSuccess == 0) {
        JLOG(ERR, "JPU_DecGetOutputInfo decode fail\n");
        ret = JPG_RET_FAILURE;
        goto SEND_DEC_STREAM_ERR;
    }

    JLOG(INFO, "%02d  %8d     %8x %8x %10d  %8x  %8x %10d\n",
        pJpgInst->instIndex, pjpgHandle->outputInfo.indexFrameDisplay,
        pjpgHandle->outputInfo.bytePosFrameStart,
        pjpgHandle->outputInfo.ecsPtr,
        pjpgHandle->outputInfo.consumedByte,
        pjpgHandle->outputInfo.rdPtr,
        pjpgHandle->outputInfo.wrPtr, pjpgHandle->outputInfo.frameCycle);

    if (pjpgHandle->outputInfo.indexFrameDisplay == -1)
        goto SEND_DEC_STREAM_ERR;

    if (pjpgHandle->outputInfo.numOfErrMBs) {
        Int32 errRstIdx, errPosX, errPosY;
        errRstIdx = (pjpgHandle->outputInfo.numOfErrMBs & 0x0F000000) >> 24;
        errPosX = (pjpgHandle->outputInfo.numOfErrMBs & 0x00FFF000) >> 12;
        errPosY = (pjpgHandle->outputInfo.numOfErrMBs & 0x00000FFF);
        JLOG(ERR, "Error restart Idx : %d, MCU x:%d, y:%d \n", errRstIdx, errPosX, errPosY);
    }

    if(pjpgHandle->outputInfo.indexFrameDisplay >= 0)
        pjpgHandle->i32ValidCnt++;

SEND_DEC_STREAM_ERR:
    if(pjpgHandle->iCoreIdx >= 0) {
        JpgEnterLockEx();
        JpgResReleaseCore(pjpgHandle->iCoreIdx);
        pJpgInst->coreIndex = -1;
        pjpgHandle->iCoreIdx = -1;
        JpgLeaveLockEx();
    }

    return ret;
}

int sophgo_jpeg_dec_get_frame_data(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, SOPHGO_S_FRAME_BUF *pstFrameBuf)
{
    SOPHGO_JPEG_DEC_HANDLE_INFO *pjpgHandle = (SOPHGO_JPEG_DEC_HANDLE_INFO *)pvJpgHandle;
    JpgInst *pJpgInst = (JpgInst *)pjpgHandle->pHandle;
    JpgDecOutputInfo *outputInfo = &pjpgHandle->outputInfo;
    JpgDecInfo *pDecInfo = &pJpgInst->JpgInfo->decInfo;

    CHECK_PTR_RET_INT(pjpgHandle);
    CHECK_PTR_RET_INT(pstFrameBuf);
    if (outputInfo->indexFrameDisplay == -1) {
        JLOG(ERR, "sophgo_jpeg_dec_get_frame_data indexFrameDisplay < 0, error \n");
        return JPG_RET_FRAME_NOT_COMPLETE;
    }

    // store yuv image
    pstFrameBuf->eFormat = (pjpgHandle->decOP.outputFormat == FORMAT_MAX)? (SOPHGO_E_FRAME_FORMAT)pDecInfo->format: pjpgHandle->decOP.outputFormat;
    pstFrameBuf->ePackedFormat = (SOPHGO_E_PACKED_FORAMT)pDecInfo->packedFormat;
    pstFrameBuf->eCbcrInterleave = (SOPHGO_E_CBCR_INTERLEAVE)pDecInfo->chromaInterleave;

    if (pjpgHandle->bExternalMem) {
        pstFrameBuf->vbY.phys_addr = pjpgHandle->frameBuf[outputInfo->indexFrameDisplay].bufY;
        pstFrameBuf->vbY.virt_addr = (unsigned long)phys_to_virt(pstFrameBuf->vbY.phys_addr);
        pstFrameBuf->vbY.u32Size = pjpgHandle->frameBuf[outputInfo->indexFrameDisplay].stride *
            pjpgHandle->frameBuf[outputInfo->indexFrameDisplay].fbLumaHeight;

        pstFrameBuf->vbCb.phys_addr = pjpgHandle->frameBuf[outputInfo->indexFrameDisplay].bufCb;
        pstFrameBuf->vbCb.virt_addr = (unsigned long)phys_to_virt(pstFrameBuf->vbCb.phys_addr);
        pstFrameBuf->vbCb.u32Size = pjpgHandle->frameBuf[outputInfo->indexFrameDisplay].strideC *
            pjpgHandle->frameBuf[outputInfo->indexFrameDisplay].fbChromaHeight;

        if (!pDecInfo->chromaInterleave) {
            pstFrameBuf->vbCr.phys_addr = pjpgHandle->frameBuf[outputInfo->indexFrameDisplay].bufCr;
            pstFrameBuf->vbCr.virt_addr = (unsigned long)phys_to_virt(pstFrameBuf->vbCr.phys_addr);
            pstFrameBuf->vbCr.u32Size = pjpgHandle->frameBuf[outputInfo->indexFrameDisplay].strideC *
                pjpgHandle->frameBuf[outputInfo->indexFrameDisplay].fbChromaHeight;
        }
    } else {
        memcpy(&(pstFrameBuf->vbY), &(pjpgHandle->pFrame[outputInfo->indexFrameDisplay]->vbY),
               sizeof(jpu_buffer_t));
        memcpy(&(pstFrameBuf->vbCb), &(pjpgHandle->pFrame[outputInfo->indexFrameDisplay]->vbCb),
               sizeof(jpu_buffer_t));
        memcpy(&(pstFrameBuf->vbCr), &(pjpgHandle->pFrame[outputInfo->indexFrameDisplay]->vbCr),
               sizeof(jpu_buffer_t));
    }

    if (1 == pDecInfo->rotationIndex || 3 == pDecInfo->rotationIndex) {
        pstFrameBuf->s32Width =
            outputInfo->decPicHeight; // pDecInfo->picHeight;
        pstFrameBuf->s32Height =
            outputInfo->decPicWidth; // pDecInfo->picWidth;
    } else {
        pstFrameBuf->s32Width = pDecInfo->picWidth;
        pstFrameBuf->s32Height = pDecInfo->picHeight;
        if (pDecInfo->iHorScaleMode || pDecInfo->iVerScaleMode) {
            pstFrameBuf->s32Width = outputInfo->decPicWidth;
            pstFrameBuf->s32Height = outputInfo->decPicHeight;
        }
    }

    pstFrameBuf->s32StrideY = pjpgHandle->frameBuf[outputInfo->indexFrameDisplay].stride;
    pstFrameBuf->s32StrideC = pjpgHandle->frameBuf[outputInfo->indexFrameDisplay].strideC;
    outputInfo->indexFrameDisplay = -1;
    if(pjpgHandle->i32ValidCnt > 0)
        pjpgHandle->i32ValidCnt--;

    return 0;
}


int sophgo_jpeg_dec_release_frame_data(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle)
{
    SOPHGO_JPEG_DEC_HANDLE_INFO *pjpgHandle = (SOPHGO_JPEG_DEC_HANDLE_INFO *)pvJpgHandle;
    JpgInst *pJpgInst;
    JpgRet ret = JPG_RET_SUCCESS;

    CHECK_PTR_RET_INT(pjpgHandle);
    pJpgInst = (JpgInst *)pjpgHandle->pHandle;

    /* check handle valid */
    ret = CheckJpgInstValidity(pjpgHandle->pHandle);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(INFO, "CheckJpgInstValidity fail, return %d\n", ret);
        return ret;
    }
    JLOG(INFO,"jpg dec release E:%d idx:%d, base:0x%lx\n"
        , pjpgHandle->bExternalMem, pJpgInst->instIndex, pjpgHandle->vbBitstream.base);
    if (!pjpgHandle->bExternalMem)
        FreeFrameBuffer(pJpgInst->instIndex);
    if (pjpgHandle->vbBitstream.base) {
        jdi_free_dma_memory(&pjpgHandle->vbBitstream);
    }
    return ret;
}

int sophgo_jpeg_dec_get_valid_frame_count(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle)
{
    SOPHGO_JPEG_DEC_HANDLE_INFO *pjpgHandle = (SOPHGO_JPEG_DEC_HANDLE_INFO *)pvJpgHandle;

    CHECK_PTR_RET_INT(pjpgHandle);

    return pjpgHandle->i32ValidCnt;
}



