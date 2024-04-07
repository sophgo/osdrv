#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <uapi/linux/sched/types.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "vpuapi.h"
#include "debug.h"
#include "chagall.h"
#include "cvi_h265_interface.h"
#include "linux/mutex.h"
#include "module_common.h"
#include "base_ctx.h"
#include "vb.h"
#include "cvi_venc_help.h"
#include "cvi_venc_rc.h"

extern VB_BLK vb_phys_addr2handle(uint64_t u64PhyAddr);
extern wait_queue_head_t tVencWaitQueue[];
static DEFINE_MUTEX(__venc_init_mutex);
extern int32_t base_ion_cache_flush(uint64_t addr_p, void *addr_v, uint32_t u32Len);

#define MAX_SRC_BUFFER_NUM 32
#define MAX_ENCODE_HEADER_BUF_SIZE (1024*1024)
#define EXTERN_SRC_BUFFER_CNT 2
#define MAX_RETRY_TIMES 5

// frmame delay = preset Gop size - 1
static int presetGopDelay[] = {
    0,  /* Custom GOP, Not used */
    0,  /* All Intra */
    0,  /* IPP Cyclic GOP size 1 */
    0,  /* IBB Cyclic GOP size 1 */
    1,  /* IBP Cyclic GOP size 2 */
    3,  /* IBBBP Cyclic GOP size 4 */
    3,  /* IPPPP Cyclic GOP size 4 */
    3,  /* IBBBB Cyclic GOP size 4 */
    7,  /* IBBBB Cyclic GOP size 8 */
    0,  /* IPP_SINGLE Cyclic GOP size 1 */
};

typedef struct __cviEncParam
{
    // for idr frame
    BOOL idr_request;
    // for enable dir
    BOOL enable_idr;

    // int frameRateInfo;
    // int bitrate;

    // custom map
    BOOL isCustomMapFlag;
    BOOL edge_api_enable;
    WaveCustomMapOpt customMapOpt;

    // user data
    Uint32 userDataBufSize;
    struct list_head userdataList;

    // rotation and mirr
    Uint32 rotationAngle;
    Uint32 mirrorDirection;

    // rcmode
    Uint32 rcMode;
    Uint32 iqp;
    Uint32 pqp;

    // vui rbsp
    vpu_buffer_t vbVuiRbsp;
} cviEncParam;

typedef struct _CVI_FRAME_ATTR
{
    PhysicalAddress buffer_addr;
    uint64_t pts;
    uint64_t dts;
    int idx;
    PhysicalAddress custom_map_addr;
} CVI_FRAME_ATTR;

typedef struct encoder_handle
{
    EncHandle handle;
    EncOpenParam open_param;
    FrameBuffer *pst_frame_buffer;
    int stride;
    int core_idx;
    int channel_index;
    int min_recon_frame_count;
    int min_src_frame_count;
    int frame_idx;
    int header_encoded;
    PhysicalAddress bitstream_buffer[MAX_SRC_BUFFER_NUM];
    void *thread_handle;
    int stop_thread;
    Queue *free_stream_buffer;
    Queue *stream_packs;
    stPack backupPack;
    stChnVencInfo chn_venc_info;
    CVI_FRAME_ATTR input_frame[MAX_SRC_BUFFER_NUM];
    cviEncParam cvi_enc_param;
    int src_end;
    int ouput_end;
    int cmd_queue_full;
    int bframe_delay;
    int first_frame_pts;
    int cmd_queue_depth;
    int is_bind_mode;   // for edge is always 0
    int is_isolate_send;
    struct completion semGetStreamCmd;
    struct completion semEncDoneCmd;
    int virtualIPeriod;
    cviRoiParam roi_rect[MAX_NUM_ROI];
    int last_frame_qp;
    Queue* customMapBuffer;
    int customMapBufferSize;
    stRcInfo rc_info;
    int enable_cvi_rc;
} ENCODER_HANDLE;

static int cviInsertUserData(void *handle);

Int32 set_open_param(EncOpenParam *pst_open_param, cviInitEncConfig *pst_init_cfg)
{
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    int i = 0;

    if (pst_init_cfg->codec == CODEC_H264)
        pst_open_param->bitstreamFormat = STD_AVC;
    else
        pst_open_param->bitstreamFormat = STD_HEVC;

    pst_open_param->picWidth = pst_init_cfg->width;
    pst_open_param->picHeight = pst_init_cfg->height;
    pst_open_param->frameRateInfo = pst_init_cfg->framerate;
    pst_open_param->MESearchRange = 3;
    pst_open_param->rcInitDelay = pst_init_cfg->initialDelay;
    pst_open_param->vbvBufferSize = 0;
    pst_open_param->meBlkMode = 0;
    pst_open_param->frameSkipDisable = 1;
    pst_open_param->sliceMode.sliceMode = 0;
    pst_open_param->sliceMode.sliceSizeMode = 1;
    pst_open_param->sliceMode.sliceSize  = 115;
    pst_open_param->intraRefreshNum = 0;
    pst_open_param->rcIntraQp = 30;
    pst_open_param->userQpMax = -1;
    pst_open_param->userGamma = (Uint32)(0.75 * 32768);
    pst_open_param->rcIntervalMode = 1;
    pst_open_param->mbInterval = 0;
    pst_open_param->MEUseZeroPmv = 0;
    pst_open_param->intraCostWeight = 400;
    pst_open_param->bitRate = pst_init_cfg->bitrate * 1024;
    pst_open_param->srcBitDepth = 8;
    pst_open_param->subFrameSyncMode = 1;
    pst_open_param->frameEndian = VPU_FRAME_ENDIAN;
    pst_open_param->streamEndian = VPU_STREAM_ENDIAN;
    pst_open_param->sourceEndian = VPU_SOURCE_ENDIAN;
    pst_open_param->lineBufIntEn = 1;
    pst_open_param->bitstreamBufferSize = pst_init_cfg->bitstreamBufferSize;
    if (pst_open_param->bitstreamBufferSize == 0)
        pst_open_param->bitstreamBufferSize = CVI_H26X_DEFAULT_BUFSIZE; // default: 4MB

    pst_open_param->enablePTS = FALSE;
    pst_open_param->cmdQueueDepth = pst_init_cfg->s32CmdQueueDepth;

    /* for cvi rate control param */
    pst_open_param->frmLostMode = 1; // only support P_SKIP
    pst_open_param->statTime = pst_init_cfg->statTime;
    pst_open_param->ipQpDelta = pst_init_cfg->s32IPQpDelta;
    pst_open_param->bgQpDelta = pst_init_cfg->s32BgQpDelta;
    pst_open_param->rcMode = pst_init_cfg->rcMode;

    /* for wave521 */
    /* hevc: let firmware determines a profile according to internalbitdepth */
    /* avc: profile cannot be set by host application*/
    param->profile = pst_init_cfg->u32Profile;
    param->level = 0;
    param->tier = 0;
    param->internalBitDepth = 8;
    param->losslessEnable = 0;
    param->constIntraPredFlag = 0;
    param->useLongTerm = 0;

    /* for CMD_ENC_SEQ_GOP_PARAM */
    /* todo: PRESET_IDX_CUSTOM_GOP */
    param->gopPresetIdx = pst_init_cfg->u32GopPreset;
    if (pst_init_cfg->u32GopPreset == PRESET_IDX_CUSTOM_GOP) {
        memcpy(&param->gopParam, &pst_init_cfg->gopParam, sizeof(CustomGopParam));
    }

    /* for CMD_ENC_SEQ_INTRA_PARAM */
    param->decodingRefreshType = pst_init_cfg->decodingRefreshType;
    param->intraPeriod = pst_init_cfg->gop;
    param->avcIdrPeriod = pst_init_cfg->gop;
    param->intraQP = 0;

    /* for CMD_ENC_SEQ_CONF_WIN_TOP_BOT/LEFT_RIGHT */
    // param->confWin.top = pst_init_cfg->confWin.top;
    // param->confWin.bottom = pst_init_cfg->confWin.bottom;
    // param->confWin.left = pst_init_cfg->confWin.left;
    // param->confWin.right = pst_init_cfg->confWin.right;

    /* for CMD_ENC_SEQ_INDEPENDENT_SLICE */
    param->independSliceMode = 0;
    param->independSliceModeArg = 0;

    /* for CMD_ENC_SEQ_DEPENDENT_SLICE */
    param->dependSliceMode = 0;
    param->dependSliceModeArg = 0;

    /* for CMD_ENC_SEQ_INTRA_REFRESH_PARAM */
    param->intraRefreshMode = 0;
    param->intraRefreshArg = 0;
    param->useRecommendEncParam = pst_init_cfg->s32EncMode;

    /* for CMD_ENC_PARAM */
    if (param->useRecommendEncParam != 1)  {
        param->scalingListEnable = 0;
        param->tmvpEnable = 1;
        param->wppEnable = 0;
        param->maxNumMerge = 2;
        param->disableDeblk = 0;
        param->lfCrossSliceBoundaryEnable = 1;
        param->betaOffsetDiv2 = 0;
        param->tcOffsetDiv2 = 0;
        param->skipIntraTrans = 1;
        param->saoEnable = 1;
        param->intraNxNEnable = 1;
    }

    /* for CMD_ENC_RC_PARAM */
    pst_open_param->rcEnable = (pst_init_cfg->bitrate == 0) ? FALSE : TRUE;;
    pst_open_param->vbvBufferSize = 3000;
    param->roiEnable = 1;
    param->bitAllocMode = 0;
    for (i = 0; i < MAX_GOP_NUM; i++)
        param->fixedBitRatio[i] = 1;
    param->cuLevelRCEnable = 1;
    param->hvsQPEnable = 1;
    param->hvsQpScale = 2;

    /* for CMD_ENC_RC_MIN_MAX_QP */
    param->minQpI = 8;
    param->maxQpI = 51;
    param->minQpP = 8;
    param->maxQpP = 51;
    param->minQpB = 8;
    param->maxQpB = 51;
    param->hvsMaxDeltaQp = 10;

    /* for CMD_ENC_CUSTOM_GOP_PARAM */
    param->gopParam.customGopSize    = 0;
    for (i = 0; i < param->gopParam.customGopSize; i++) {
        param->gopParam.picParam[i].picType = PIC_TYPE_I;
        param->gopParam.picParam[i].pocOffset = 1;
        param->gopParam.picParam[i].picQp = 30;
        param->gopParam.picParam[i].refPocL0 = 0;
        param->gopParam.picParam[i].refPocL1 = 0;
        param->gopParam.picParam[i].temporalId  = 0;
    }

    // for VUI / time information.
    param->numTicksPocDiffOne = 0;
    param->timeScale = pst_open_param->frameRateInfo * 1000;
    param->numUnitsInTick  = 1000;

    param->chromaCbQpOffset = 0;
    param->chromaCrQpOffset = 0;
    param->initialRcQp = 30;
    param->nrYEnable = 0;
    param->nrCbEnable = 0;
    param->nrCrEnable = 0;
    param->nrNoiseEstEnable = 0;
    param->useLongTerm = (pst_init_cfg->virtualIPeriod > 0) ? 1 : 0;

    param->monochromeEnable = 0;
    param->strongIntraSmoothEnable = 1;
    param->weightPredEnable = 0;
    param->bgDetectEnable = 0;
    param->bgThrDiff = 8;
    param->bgThrMeanDiff = 1;
    param->bgLambdaQp = 32;
    param->bgDeltaQp = 3;

    param->customLambdaEnable = 0;
    param->customMDEnable = 0;
    param->pu04DeltaRate = 0;
    param->pu08DeltaRate = 0;
    param->pu16DeltaRate = 0;
    param->pu32DeltaRate = 0;
    param->pu04IntraPlanarDeltaRate = 0;
    param->pu04IntraDcDeltaRate = 0;
    param->pu04IntraAngleDeltaRate = 0;
    param->pu08IntraPlanarDeltaRate = 0;
    param->pu08IntraDcDeltaRate = 0;
    param->pu08IntraAngleDeltaRate = 0;
    param->pu16IntraPlanarDeltaRate = 0;
    param->pu16IntraDcDeltaRate = 0;
    param->pu16IntraAngleDeltaRate = 0;
    param->pu32IntraPlanarDeltaRate = 0;
    param->pu32IntraDcDeltaRate = 0;
    param->pu32IntraAngleDeltaRate = 0;
    param->cu08IntraDeltaRate = 0;
    param->cu08InterDeltaRate = 0;
    param->cu08MergeDeltaRate = 0;
    param->cu16IntraDeltaRate = 0;
    param->cu16InterDeltaRate = 0;
    param->cu16MergeDeltaRate = 0;
    param->cu32IntraDeltaRate = 0;
    param->cu32InterDeltaRate = 0;
    param->cu32MergeDeltaRate = 0;
    param->coefClearDisable = 0;

    param->rcWeightParam               = 2;
    param->rcWeightBuf                 = 128;
    param->s2SearchRangeXDiv4          = 32;
    param->s2SearchRangeYDiv4          = 16;

    // for H.264 encoder
    param->avcIdrPeriod =
        ((param->gopPresetIdx == 1) && (pst_open_param->bitstreamFormat == STD_AVC)) ? 1 : param->avcIdrPeriod;
    param->rdoSkip = 1;
    param->lambdaScalingEnable = 1;

    param->transform8x8Enable = 1;
    param->avcSliceMode = 0;
    param->avcSliceArg = 0;
    param->intraMbRefreshMode = 0;
    param->intraMbRefreshArg = 1;
    param->mbLevelRcEnable = 1;
    param->entropyCodingMode = 1;
    param->disableDeblk = 0;

    return 0;
}

void cviVcodecGetVersion(void)
{

    return;
}

int set_vb_flag(PhysicalAddress addr)
{
    VB_BLK blk;
    struct vb_s *vb;

    blk = vb_phys_addr2handle(addr);
    if (blk == VB_INVALID_HANDLE)
        return 0;

    vb = (struct vb_s *)&blk;
    atomic_fetch_add(1, &vb->usr_cnt);
    atomic_long_fetch_or(BIT(CVI_ID_VENC), &vb->mod_ids);

    return 0;
}

int clr_vb_flag(PhysicalAddress addr)
{
    VB_BLK blk;
    struct vb_s *vb;

    blk = vb_phys_addr2handle(addr);
    if (blk == VB_INVALID_HANDLE)
        return 0;

    vb = (struct vb_s *)&blk;
    atomic_long_fetch_and(~BIT(CVI_ID_VENC), &vb->mod_ids);
    vb_release_block(blk);

    return 0;
}

static int get_frame_idx(void * handle, PhysicalAddress addr)
{
    int idx;
    ENCODER_HANDLE *pst_handle = handle;

    for(idx=0; idx<MAX_SRC_BUFFER_NUM; idx++) {
        if (pst_handle->input_frame[idx].buffer_addr == 0) {
            pst_handle->input_frame[idx].buffer_addr = addr;
            break;
        }

        if (pst_handle->input_frame[idx].buffer_addr == addr)
            break;
    }

    if (idx == MAX_SRC_BUFFER_NUM)
        return -1;

    return idx;
}

static void release_frame_idx(void * handle, int srcIdx)
{
    ENCODER_HANDLE *pst_handle = handle;

    // sanity check
    if (!pst_handle || srcIdx >= MAX_SRC_BUFFER_NUM) {
        return;
    }

    pst_handle->input_frame[srcIdx].buffer_addr = 0;
}

static int  alloc_framebuffer(void * handle)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    cviEncParam *pst_cvi_param = &pst_handle->cvi_enc_param;
    Uint32 fbWidth = 0, fbHeight = 0;
    int stride;
    int frame_size;
    int i;
    int ret;
    vpu_buffer_t vb_buffer;
    int map_type = COMPRESSED_FRAME_MAP;

    if (pst_open_param->bitstreamFormat == STD_AVC) {
        fbWidth  = VPU_ALIGN16(pst_open_param->picWidth);
        fbHeight = VPU_ALIGN16(pst_open_param->picHeight);

        if ((pst_cvi_param->rotationAngle != 0 || pst_cvi_param->mirrorDirection != 0)
            && !(pst_cvi_param->rotationAngle == 180 && pst_cvi_param->mirrorDirection == MIRDIR_HOR_VER)) {
            fbWidth  = VPU_ALIGN16(pst_open_param->picWidth);
            fbHeight = VPU_ALIGN16(pst_open_param->picHeight);
        }
        if (pst_cvi_param->rotationAngle == 90 || pst_cvi_param->rotationAngle == 270) {
            fbWidth  = VPU_ALIGN16(pst_open_param->picHeight);
            fbHeight = VPU_ALIGN16(pst_open_param->picWidth);
        }
    } else {
        fbWidth  = VPU_ALIGN8(pst_open_param->picWidth);
        fbHeight = VPU_ALIGN8(pst_open_param->picHeight);

        if ((pst_cvi_param->rotationAngle != 0 || pst_cvi_param->mirrorDirection != 0)
            && !(pst_cvi_param->rotationAngle == 180 && pst_cvi_param->mirrorDirection == MIRDIR_HOR_VER)) {
            fbWidth  = VPU_ALIGN32(pst_open_param->picWidth);
            fbHeight = VPU_ALIGN32(pst_open_param->picHeight);
        }
        if (pst_cvi_param->rotationAngle == 90 || pst_cvi_param->rotationAngle == 270) {
            fbWidth  = VPU_ALIGN32(pst_open_param->picHeight);
            fbHeight = VPU_ALIGN32(pst_open_param->picWidth);
        }
    }

    pst_handle->pst_frame_buffer = (FrameBuffer *)vzalloc(pst_handle->min_recon_frame_count * sizeof(FrameBuffer));
    stride = VPU_GetFrameBufStride(pst_handle->handle, fbWidth, fbHeight,
        FORMAT_420, 0, map_type);
    frame_size = VPU_GetFrameBufSize(pst_handle->handle, pst_handle->core_idx, stride,
        fbHeight, map_type, FORMAT_420, 0, NULL);
    vb_buffer.size = frame_size;

    for (i = 0; i < pst_handle->min_recon_frame_count; i++) {
        ret = vdi_allocate_dma_memory(pst_handle->core_idx, &vb_buffer, 0, 0);
        if(ret != RETCODE_SUCCESS) {
            return ret;
        }
        pst_handle->pst_frame_buffer[i].bufY = vb_buffer.phys_addr;
        pst_handle->pst_frame_buffer[i].bufCb = (PhysicalAddress) - 1;
        pst_handle->pst_frame_buffer[i].bufCr = (PhysicalAddress) - 1;
        pst_handle->pst_frame_buffer[i].updateFbInfo = 1;
        pst_handle->pst_frame_buffer[i].size = vb_buffer.size;
    }

    ret = VPU_EncRegisterFrameBuffer(pst_handle->handle, pst_handle->pst_frame_buffer,
        pst_handle->min_recon_frame_count, stride, fbHeight, map_type);
    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "Failed VPU_EncRegisterFrameBuffer(ret:%d)\n", ret);
        return ret;
    }

    return RETCODE_SUCCESS;
}

static int cviCheckIdrPeriod(void *handle)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    int isIframe;

    if (pst_handle->frame_idx == 0) {
        return TRUE;
    } else if (param->intraPeriod == 0) {
        return FALSE;
    }

    isIframe = ((pst_handle->frame_idx % param->intraPeriod) == 0);

    return isIframe;
}

static void cviPicParamChangeCtrl(void *handle, EncParam *encParam)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    cviEncParam *pst_cvi_param = &pst_handle->cvi_enc_param;
    EncChangeParam  changeParam;
    int enable_option = 0;
    int ret = 0;
    BOOL rateChangeCmd = FALSE;

    osal_memset(&changeParam, 0x00, sizeof(EncChangeParam));

    // W5_ENC_CHANGE_PARAM_RC_TARGET_RATE
    if(pst_open_param->rcMode == RC_MODE_AVBR) {  //no need to refresh at idr frame in avbr mode
        if(cviEnc_Avbr_PicCtrl(&pst_handle->rc_info, pst_open_param, pst_handle->frame_idx)) {
            cviEncRc_SetParam(&pst_handle->rc_info, pst_open_param, E_BITRATE);
            enable_option |= W5_ENC_CHANGE_PARAM_RC_TARGET_RATE;
            changeParam.bitRate = pst_handle->rc_info.targetBitrate;
            rateChangeCmd = TRUE;
        }
    } else {
        if(encParam->is_idr_frame && pst_open_param->bitRate != cviEncRc_GetParam(&pst_handle->rc_info, E_BITRATE)) {
            cviEncRc_SetParam(&pst_handle->rc_info, pst_open_param, E_BITRATE);
            enable_option |= W5_ENC_CHANGE_PARAM_RC_TARGET_RATE;
            changeParam.bitRate = pst_handle->rc_info.targetBitrate;
            rateChangeCmd = TRUE;
        }
    }

    // framerate change
    if (encParam->is_idr_frame && pst_open_param->frameRateInfo != cviEncRc_GetParam(&pst_handle->rc_info, E_FRAMERATE)) {
        cviEncRc_SetParam(&pst_handle->rc_info, pst_open_param, E_FRAMERATE);
        enable_option |= W5_ENC_CHANGE_PARAM_RC_FRAME_RATE;
        changeParam.frameRate = pst_handle->rc_info.framerate;
        rateChangeCmd = TRUE;
    }

    if (rateChangeCmd) {
        changeParam.enable_option = enable_option;
        ret = VPU_EncGiveCommand(pst_handle->handle, ENC_SET_PARA_CHANGE, &changeParam);
    }

    if (pst_cvi_param->enable_idr == FALSE && pst_handle->frame_idx != 0) {
        encParam->is_idr_frame = FALSE;
    }

    // idr request
    if (pst_cvi_param->idr_request == TRUE && encParam->is_idr_frame == FALSE) {
        encParam->is_idr_frame = TRUE;
        pst_cvi_param->idr_request = FALSE;

        encParam->forcePicTypeEnable = 1;
        encParam->forcePicType = 3;    // IDR
    }

    if (pst_handle->rc_info.rcEnable && pst_handle->rc_info.rcMode == RC_MODE_AVBR) {
        if (pst_handle->rc_info.avbrChangeBrEn == TRUE) {
            int deltaQp = cviEncRc_Avbr_GetQpDelta(&pst_handle->rc_info, pst_open_param);
            int maxQp = encParam->is_idr_frame ? param->maxQpI : param->maxQpP;
            int minQp = encParam->is_idr_frame ? param->minQpI : param->minQpP;

            changeParam.enable_option = W5_ENC_CHANGE_PARAM_RC_MIN_MAX_QP;
            changeParam.maxQpI = CLIP3(0, 51, maxQp + deltaQp);
            changeParam.minQpI = CLIP3(0, 51, minQp);
            changeParam.hvsMaxDeltaQp = deltaQp;
            VPU_EncGiveCommand(pst_handle->handle, ENC_SET_PARA_CHANGE, &changeParam);
        }
    }
}

static int cviInsertOneUserDataSegment(Queue *psp, Uint8 *pUserData,
                       Uint32 userDataLen)
{
    stPack userdata_pack = {0};
    Uint8 *pst_buffer = NULL;
    uint32_t total_packs = 0;

    total_packs = Queue_Get_Cnt(psp);
    if (total_packs >= MAX_NUM_PACKS) {
        VLOG(ERR, "totalPacks (%d) >= MAX_NUM_PACKS\n", total_packs);
        return FALSE;
    }

    pst_buffer = (Uint8 *)osal_kmalloc(userDataLen);
    if (pst_buffer == NULL) {
        VLOG(ERR, "out of memory\n");
        return FALSE;
    }

    memcpy(pst_buffer, pUserData, userDataLen);
    memset(&userdata_pack, 0, sizeof(stPack));

    userdata_pack.addr = pst_buffer;
    userdata_pack.len = userDataLen;
    userdata_pack.cviNalType = NAL_SEI;
    userdata_pack.need_free = TRUE;
    userdata_pack.u64PhyAddr = virt_to_phys(pst_buffer);
    vdi_flush_ion_cache(userdata_pack.u64PhyAddr, pst_buffer, userDataLen);

    Queue_Enqueue(psp, &userdata_pack);

    return TRUE;
}

static int cviInsertUserData(void *handle)
{
    int ret = TRUE;
    ENCODER_HANDLE *pst_handle = handle;
    cviEncParam *pst_cvi_param = &pst_handle->cvi_enc_param;
    UserDataList *userdataNode = NULL;
    UserDataList *n;

    list_for_each_entry_safe(userdataNode, n, &pst_cvi_param->userdataList, list) {
        if (userdataNode->userDataBuf != NULL && userdataNode->userDataLen != 0) {
            if (!cviInsertOneUserDataSegment(pst_handle->stream_packs,
                             userdataNode->userDataBuf,
                             userdataNode->userDataLen)) {
                VLOG(ERR, "failed to insert user data\n");
                ret = FALSE;
            }
            osal_free(userdataNode->userDataBuf);
            list_del(&userdataNode->list);
            osal_free(userdataNode);
            return ret;
        }
    }

    return ret;
}

Int32 writeVuiRbspData(void *handle,  Uint8 *pVuiRbspBuf, int32_t vui_len)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    cviEncParam *pst_cvi_param = &pst_handle->cvi_enc_param;
    int ret = 0;

    if (pst_open_param->encodeVuiRbsp) {
        pst_cvi_param->vbVuiRbsp.size = VUI_HRD_RBSP_BUF_SIZE;

        if (vdi_allocate_dma_memory(pst_handle->core_idx, &pst_cvi_param->vbVuiRbsp, ENC_ETC, 0) < 0) {
            VLOG(ERR, "fail to allocate VUI rbsp buffer\n" );
            return FALSE;
        }
        pst_open_param->vuiRbspDataAddr = pst_cvi_param->vbVuiRbsp.phys_addr;
        ret = vdi_write_memory(pst_handle->core_idx, pst_open_param->vuiRbspDataAddr
            , pVuiRbspBuf, VPU_ALIGN16(vui_len), VDI_128BIT_BIG_ENDIAN);
    }

    return TRUE;
}

static int cviH264SpsAddVui(void *handle, cviH264Vui *pVui)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    char *vui_rbsp_addr = NULL;
    int vui_rbsp_byte_len = 0, vui_rbsp_bit_len = 0;

    H264SpsAddVui(pVui, (void **)&vui_rbsp_addr, &vui_rbsp_byte_len, &vui_rbsp_bit_len);
    if (!vui_rbsp_addr || vui_rbsp_byte_len == 0) {
        return -1;
    }

    pst_open_param->encodeVuiRbsp = 1;
    pst_open_param->vuiRbspDataSize = vui_rbsp_bit_len;
    writeVuiRbspData(handle, vui_rbsp_addr, vui_rbsp_byte_len);

    if (vui_rbsp_addr) {
        osal_kfree(vui_rbsp_addr);
    }
    return 0;
}

static int cviH265SpsAddVui(void *handle, cviH265Vui *pVui)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    void *vui_rbsp_addr = NULL;
    int vui_rbsp_byte_len = 0, vui_rbsp_bit_len = 0;

    H265SpsAddVui(pVui, &vui_rbsp_addr, &vui_rbsp_byte_len, &vui_rbsp_bit_len);
    if (!vui_rbsp_addr || vui_rbsp_byte_len == 0) {
        return -1;
    }

    pst_open_param->encodeVuiRbsp = 1;
    pst_open_param->vuiRbspDataSize = vui_rbsp_bit_len;
    writeVuiRbspData(handle, vui_rbsp_addr, vui_rbsp_byte_len);

    if (vui_rbsp_addr) {
        osal_kfree(vui_rbsp_addr);
    }
    return 0;
}

static int thread_wait_interrupt(void *param)
{
    ENCODER_HANDLE *pst_handle = param;
    EncInitialInfo init_info = {0};
    QueueStatusInfo queue_status = {0};
    EncOutputInfo output_info;
    int ret;
    int retry_times = 0;
    int cyclePerTick = 256;
    stPack encode_pack = {0};

    while (1) {
        if (pst_handle->stop_thread) {
            VPU_EncGiveCommand(pst_handle->handle, ENC_GET_QUEUE_STATUS, &queue_status);
            if (!queue_status.instanceQueueCount && queue_status.reportQueueEmpty)
                break;
        }

        retry_times = 0;
        ret = VPU_WaitInterruptEx(pst_handle->handle, 100);
        if (ret == -1)
            continue;

        if (ret > 0) {
            VPU_ClearInterruptEx(pst_handle->handle, ret);

            if (ret & (1 << INT_WAVE5_ENC_SET_PARAM)) {
                ret = VPU_EncCompleteSeqInit(pst_handle->handle, &init_info);
                if (ret == RETCODE_VPU_RESPONSE_TIMEOUT) {
                    VLOG(ERR, "<%s:%d> Failed to VPU_EncCompleteSeqInit()\n", __FUNCTION__, __LINE__);
                    break;
                }
            }

            if (ret & (1 << INT_WAVE5_ENC_PIC)) {
                memset(&output_info, 0, sizeof(EncOutputInfo));
                do {
                    ret = VPU_EncGetOutputInfo(pst_handle->handle, &output_info);
                    if (ret != RETCODE_SUCCESS) {
                        VLOG(TRACE, "VPU_EncGetOutputInfo ret:%d, pictype:0x%x, streamSize:%d\n"
                            , ret, output_info.picType, output_info.bitstreamSize);
                        retry_times++;
                        osal_msleep(1);
                        continue;
                    } else {
                        break;
                    }
                } while (retry_times <= MAX_RETRY_TIMES);

                if (ret == RETCODE_REPORT_NOT_READY || ret == RETCODE_QUERY_FAILURE
                        || output_info.picType >= PIC_TYPE_MAX) {
                    VLOG(TRACE, "VPU_EncGetOutputInfo continue ret:%d, pictype:0x%x\n", ret, output_info.picType);
                    continue;
                }

                if (ret != RETCODE_SUCCESS) {
                    VLOG(ERR, "Failed VPU_EncGetOutputInfo ret:%d, reason:0x%x\n", ret, output_info.errorReason);
                    break;
                }

                VLOG(TRACE, "encode type:%d bitstreamSize:%u, releaseFlag:0x%x, recon:%d \n"
                   , output_info.picType, output_info.bitstreamSize, output_info.releaseSrcFlag, output_info.reconFrameIndex);
                if (output_info.bitstreamSize > 0) {
                    // backup vps/sps/pps
                    if (output_info.reconFrameIndex == RECON_IDX_FLAG_HEADER_ONLY  && output_info.picType == PIC_TYPE_I) {
                        if (pst_handle->backupPack.len > 0 && pst_handle->backupPack.u64PhyAddr) {
                            Queue_Enqueue(pst_handle->free_stream_buffer, &pst_handle->backupPack.u64PhyAddr);
                            memset(&pst_handle->backupPack, 0, sizeof(stPack));
                        }
                        pst_handle->backupPack.u64PhyAddr = output_info.bitstreamBuffer;
                        pst_handle->backupPack.addr = phys_to_virt(output_info.bitstreamBuffer);
                        pst_handle->backupPack.len = output_info.bitstreamSize;
                        pst_handle->backupPack.encSrcIdx = RECON_IDX_FLAG_HEADER_ONLY;
                        pst_handle->backupPack.cviNalType =
                            (pst_handle->open_param.bitstreamFormat == STD_HEVC) ? NAL_VPS : NAL_SPS;;
                        pst_handle->backupPack.need_free = FALSE;
                        pst_handle->backupPack.u64PTS = 0;
                        pst_handle->backupPack.u64DTS = 0;
                        pst_handle->backupPack.bUsed = FALSE;
                        vdi_invalidate_ion_cache(pst_handle->backupPack.u64PhyAddr
                                    , pst_handle->backupPack.addr
                                    , pst_handle->backupPack.len);
                        continue;
                    }  else if (output_info.picType == PIC_TYPE_I || output_info.picType == PIC_TYPE_IDR) {
                        if (pst_handle->backupPack.len > 0) {
                            // header and idr need 2 packs
                            if (Queue_Get_Cnt(pst_handle->stream_packs) < (MAX_NUM_PACKS-1)) {
                                memcpy(&encode_pack, &pst_handle->backupPack, sizeof(stPack));
                                memset(&pst_handle->backupPack, 0, sizeof(stPack));
                                Queue_Enqueue(pst_handle->stream_packs, &encode_pack);
                            }
                        }
                    }

                    if (output_info.avgCtuQp > 0) {
                        pst_handle->last_frame_qp = output_info.avgCtuQp;
                        ret = cviInsertUserData(pst_handle);
                        if (ret == FALSE) {
                            VLOG(ERR, "cviInsertUserData failed %d\n", ret);
                        }
                    }

                    vdi_invalidate_ion_cache(output_info.bitstreamBuffer
                                            , phys_to_virt(output_info.bitstreamBuffer)
                                            , output_info.bitstreamSize);

                    // drop this packet
                    if (Queue_Get_Cnt(pst_handle->stream_packs) >= MAX_NUM_PACKS) {
                        pst_handle->chn_venc_info.dropCnt++;
                        Queue_Enqueue(pst_handle->free_stream_buffer, &output_info.bitstreamBuffer);
                        continue;
                    }

                    encode_pack.u64PhyAddr = output_info.bitstreamBuffer;
                    encode_pack.addr = phys_to_virt(output_info.bitstreamBuffer);
                    encode_pack.len = output_info.bitstreamSize;
                    encode_pack.encSrcIdx = output_info.encSrcIdx;
                    encode_pack.cviNalType = output_info.picType;
                    encode_pack.need_free = FALSE;
                    encode_pack.u64PTS = output_info.pts;
                    encode_pack.u64DTS =
                            output_info.encPicCnt - 1 - pst_handle->bframe_delay + pst_handle->first_frame_pts;
                    encode_pack.u32AvgCtuQp = output_info.avgCtuQp;
                    encode_pack.bUsed = FALSE;
                    encode_pack.u32EncHwTime =
                            (output_info.encEncodeEndTick - output_info.encHostCmdTick)*cyclePerTick/(VPU_STAT_CYCLES_CLK/1000000);
                    if (output_info.encSrcIdx >= 0 && output_info.encSrcIdx < MAX_SRC_BUFFER_NUM) {
                        encode_pack.u64CustomMapAddr = pst_handle->input_frame[output_info.encSrcIdx].custom_map_addr;
                    }
                    Queue_Enqueue(pst_handle->stream_packs, &encode_pack);
                    Queue_Enqueue(pst_handle->customMapBuffer, &pst_handle->input_frame[output_info.encSrcIdx].custom_map_addr);
                }

                if (output_info.encSrcIdx >= 0 && output_info.encSrcIdx < MAX_SRC_BUFFER_NUM) {
                    release_frame_idx(pst_handle, output_info.encSrcIdx);
                }

                if (pst_handle->src_end == 1 && output_info.reconFrameIndex == RECON_IDX_FLAG_ENC_END) {
                    pst_handle->ouput_end = 1;
                }

                pst_handle->chn_venc_info.getStreamCnt++;
                complete(&pst_handle->semEncDoneCmd);
                complete(&pst_handle->semGetStreamCmd);
                wake_up(&tVencWaitQueue[pst_handle->channel_index]);
            }

            if (ret & (1 << INT_WAVE5_BSBUF_FULL)) {
            }
        }

        cond_resched();
    }

    pst_handle->stop_thread = 1;
    pst_handle->thread_handle = NULL;
    return 0;
}

void cviVencInit(void)
{
    mutex_lock(&__venc_init_mutex);
    if (VPU_GetProductId(0) != PRODUCT_ID_521) {
        mutex_unlock(&__venc_init_mutex);
        VLOG(ERR, "<%s:%d> Failed to VPU_GetProductId()\n", __FUNCTION__, __LINE__);
        return;
    }
    mutex_unlock(&__venc_init_mutex);
}

void *cviVEncOpen(cviInitEncConfig *pInitEncCfg)
{
    BOOL ret;
    ENCODER_HANDLE *pst_handle;
    int fw_size;
    Uint16 *pus_bitCode;


    pus_bitCode = (Uint16 *)bit_code;
    fw_size = ARRAY_SIZE(bit_code);
    ret = VPU_InitWithBitcode(0, pus_bitCode, fw_size);
    if ((ret != RETCODE_SUCCESS) && (ret != RETCODE_CALLED_BEFORE)) {
        VLOG(ERR, "<%s:%d> Failed to VPU_InitWithBitcode()\n", __func__, __LINE__);
        return NULL;
    }

    pst_handle = vzalloc(sizeof(ENCODER_HANDLE));
    if (pst_handle == NULL)
        return NULL;

    set_open_param(&pst_handle->open_param, pInitEncCfg);
    pst_handle->open_param.coreIdx = pst_handle->core_idx;
    pst_handle->bframe_delay = presetGopDelay[pst_handle->open_param.EncStdParam.waveParam.gopPresetIdx];
    pst_handle->cmd_queue_depth = pInitEncCfg->s32CmdQueueDepth;
    pst_handle->is_isolate_send = pInitEncCfg->bIsoSendFrmEn;
    init_completion(&pst_handle->semGetStreamCmd);
    init_completion(&pst_handle->semEncDoneCmd);
    pst_handle->virtualIPeriod = pInitEncCfg->virtualIPeriod;

    VLOG(INFO, "<%s:%d> cmd_queue_depth:%d, isolate_send:%d, virtualIPeriod:%d\n"
        , __func__, __LINE__, pst_handle->cmd_queue_depth, pst_handle->is_isolate_send
        , pst_handle->virtualIPeriod);
    // for insertUserData
    pst_handle->cvi_enc_param.userDataBufSize = pInitEncCfg->userDataMaxLength;
    INIT_LIST_HEAD(&pst_handle->cvi_enc_param.userdataList);

    pst_handle->cvi_enc_param.enable_idr = TRUE;
    // for rotation and mirror direction
    pst_handle->cvi_enc_param.rotationAngle = pInitEncCfg->s32RotationAngle * 90;
    pst_handle->cvi_enc_param.mirrorDirection = pInitEncCfg->s32MirrorDirection;

    pst_handle->cvi_enc_param.rcMode = pInitEncCfg->rcMode;
    pst_handle->cvi_enc_param.iqp = pInitEncCfg->iqp;
    pst_handle->cvi_enc_param.pqp = pInitEncCfg->pqp;
    pst_handle->stream_packs = Queue_Create_With_Lock(MAX_NUM_PACKS, sizeof(stPack));

    pst_handle->enable_cvi_rc = 1;
    return pst_handle;
}

int cviVEncClose(void *handle)
{
    int i;
    vpu_buffer_t vb_buffer;
    ENCODER_HANDLE *pst_handle = handle;
    cviEncParam *pCviEncParam = &pst_handle->cvi_enc_param;
    UserDataList *userdataNode = NULL;
    UserDataList *n;
    int int_reason = 0;

    if (pst_handle->thread_handle != NULL) {
        pst_handle->stop_thread = 1;
        osal_thread_join(pst_handle->thread_handle, NULL);
    }

    while (VPU_EncClose(pst_handle->handle) == RETCODE_VPU_STILL_RUNNING) {
        if ((int_reason = VPU_WaitInterruptEx(pst_handle->handle, 1000)) == -1) {
            VLOG(ERR, "NO RESPONSE FROM VPU_EncClose2()\n");
            break;
        }
        else if (int_reason > 0) {
            VPU_ClearInterruptEx(pst_handle->handle, int_reason);
            if (int_reason & (1 << INT_WAVE5_ENC_PIC)) {
                EncOutputInfo   outputInfo;
                VLOG(INFO, "VPU_EncClose() : CLEAR REMAIN INTERRUPT\n");
                VPU_EncGetOutputInfo(pst_handle->handle, &outputInfo);
                continue;
            }
        }
        osal_msleep(10);
    }

    for (i = 0; i < pst_handle->min_recon_frame_count; i++) {
        if (pst_handle->pst_frame_buffer[i].size == 0)
            continue;

        vb_buffer.phys_addr = pst_handle->pst_frame_buffer[i].bufY;
        vb_buffer.size = pst_handle->pst_frame_buffer[i].size;
        vdi_free_dma_memory(pst_handle->core_idx, &vb_buffer, 0, 0);
    }

    if (pst_handle->pst_frame_buffer != NULL) {
        vfree(pst_handle->pst_frame_buffer);
        pst_handle->pst_frame_buffer = NULL;
    }

    for (i = 0; i < pst_handle->min_src_frame_count + EXTERN_SRC_BUFFER_CNT; i++) {
        if (pst_handle->bitstream_buffer[i] == 0)
            continue;

        vb_buffer.phys_addr = pst_handle->bitstream_buffer[i];
        vb_buffer.size = pst_handle->open_param.bitstreamBufferSize;
        vdi_free_dma_memory(pst_handle->core_idx, &vb_buffer, 0, 0);
    }
    while(Queue_Get_Cnt(pst_handle->customMapBuffer) > 0) {
        PhysicalAddress *phys_addr = Queue_Dequeue(pst_handle->customMapBuffer);
        vb_buffer.phys_addr = *phys_addr;
        vb_buffer.size = pst_handle->customMapBufferSize;
        vdi_free_dma_memory(pst_handle->core_idx, &vb_buffer, ENC_ETC, 0);
    }

    if (pCviEncParam->vbVuiRbsp.size) {
        vdi_free_dma_memory(pst_handle->core_idx, &pCviEncParam->vbVuiRbsp, ENC_ETC, 0);
        pCviEncParam->vbVuiRbsp.size = 0;
        pCviEncParam->vbVuiRbsp.phys_addr = 0UL;
    }

    Queue_Destroy(pst_handle->stream_packs);
    Queue_Destroy(pst_handle->free_stream_buffer);
    Queue_Destroy(pst_handle->customMapBuffer);
    VPU_DeInit(pst_handle->core_idx);

    list_for_each_entry_safe(userdataNode, n, &pCviEncParam->userdataList, list) {
        if (userdataNode->userDataBuf != NULL && userdataNode->userDataLen != 0) {
            osal_free(userdataNode->userDataBuf);
            list_del(&userdataNode->list);
            osal_free(userdataNode);
        }
    }
    pCviEncParam->userDataBufSize = 0;

    vfree(pst_handle);
    return 0;
}

int cviBuildEncodeHeader(void *handle, EncHeaderParam *pst_enc_param, BOOL is_wait, BOOL is_publish)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOutputInfo output_info;
    EncInitialInfo init_info = {0};
    // uint8_t *pVirtHeaderBuf = NULL;
    int int_reason = 0;
    int ret = 0;
    int retry_times = 0;

    if (pst_handle->open_param.bitstreamFormat == STD_HEVC)
        pst_enc_param->headerType = CODEOPT_ENC_VPS | CODEOPT_ENC_SPS | CODEOPT_ENC_PPS;
    else
        pst_enc_param->headerType = CODEOPT_ENC_SPS | CODEOPT_ENC_PPS;

    do {
        ret = VPU_EncGiveCommand(pst_handle->handle, ENC_PUT_VIDEO_HEADER, pst_enc_param);
    } while (ret == RETCODE_QUEUEING_FAILURE);

    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "Failed ENC_PUT_VIDEO_HEADER(ret:%d)\n", ret);
        return ret;
    }

    if (is_wait) {
        do {
            if (retry_times >= MAX_RETRY_TIMES) {
                break;
            }
            retry_times++;
            int_reason = VPU_WaitInterruptEx(pst_handle->handle, 100);
            if (int_reason < INTERRUPT_TIMEOUT_VALUE) {
                VLOG(ERR, "<%s:%d> Failed to VPU_WaitInterruptEx int_reason(%d)\n"
                    , __FUNCTION__, __LINE__, int_reason);
                ret = RETCODE_FAILURE;
                break;
            }

            if (int_reason > 0) {
                VPU_ClearInterruptEx(pst_handle->handle, int_reason);

                if (int_reason & (1 << INT_WAVE5_ENC_SET_PARAM)) {
                    ret = VPU_EncCompleteSeqInit(pst_handle->handle, &init_info);
                    continue;
                }

                if (int_reason & (1 << INT_WAVE5_ENC_PIC)) {
                    memset(&output_info, 0, sizeof(EncOutputInfo));
                    ret = VPU_EncGetOutputInfo(pst_handle->handle, &output_info);
                    if (ret != RETCODE_SUCCESS) {
                        VLOG(ERR, "Failed VPU_EncGetOutputInfo ret:%d, reason:0x%x\n", ret, output_info.errorReason);
                        ret = RETCODE_FAILURE;
                        break;
                    }

                    pst_enc_param->size = output_info.bitstreamSize;

                    ret = RETCODE_SUCCESS;
                    if (is_publish) {
                        if (pst_handle->backupPack.len > 0 && pst_handle->backupPack.u64PhyAddr) {
                            Queue_Enqueue(pst_handle->free_stream_buffer, &pst_handle->backupPack.u64PhyAddr);
                            memset(&pst_handle->backupPack, 0, sizeof(stPack));
                        }
                        pst_handle->backupPack.u64PhyAddr = output_info.bitstreamBuffer;
                        pst_handle->backupPack.addr = phys_to_virt(output_info.bitstreamBuffer);
                        pst_handle->backupPack.len = output_info.bitstreamSize;
                        pst_handle->backupPack.encSrcIdx = RECON_IDX_FLAG_HEADER_ONLY;
                        pst_handle->backupPack.cviNalType =
                            (pst_handle->open_param.bitstreamFormat == STD_HEVC) ? NAL_VPS : NAL_SPS;
                        pst_handle->backupPack.need_free = FALSE;
                        pst_handle->backupPack.u64PTS = 0;
                        pst_handle->backupPack.u64DTS = 0;
                        pst_handle->backupPack.bUsed = FALSE;
                        vdi_invalidate_ion_cache(pst_handle->backupPack.u64PhyAddr
                                    , pst_handle->backupPack.addr
                                    , pst_handle->backupPack.len);
                    }
                }
            }
        } while(int_reason == INTERRUPT_TIMEOUT_VALUE || !(int_reason & (1 << INT_WAVE5_ENC_PIC)) );
    }

    return ret;
}

int cviVEncGetEncodeHeader(void *handle, void *arg)
{
    int ret;
    ENCODER_HANDLE *pst_handle = handle;
    cviEncodeHeaderInfo *encHeaderRbsp = (cviEncodeHeaderInfo *)arg;
    EncHeaderParam encHeaderParam = {0};
    vpu_buffer_t vb_buffer;
    uint8_t *pVirtHeaderBuf = NULL;
    BOOL is_wait_interrupt = 1;
    BOOL is_publish = 0;

    osal_memset(&vb_buffer, 0, sizeof(vpu_buffer_t));
    vb_buffer.size = MAX_ENCODE_HEADER_BUF_SIZE;
    vdi_allocate_dma_memory(pst_handle->core_idx, &vb_buffer, 0, 0);

    osal_memset(&encHeaderParam, 0x00, sizeof(EncHeaderParam));
    encHeaderParam.buf = vb_buffer.phys_addr;
    encHeaderParam.size = MAX_ENCODE_HEADER_BUF_SIZE;

    ret = cviBuildEncodeHeader(pst_handle, &encHeaderParam, is_wait_interrupt, is_publish);
    if (ret == RETCODE_SUCCESS) {
        pVirtHeaderBuf = phys_to_virt(encHeaderParam.buf);
        vdi_invalidate_ion_cache(encHeaderParam.buf
                                    , pVirtHeaderBuf
                                    , encHeaderParam.size);
        osal_memcpy(encHeaderRbsp->headerRbsp, pVirtHeaderBuf, encHeaderParam.size);
        encHeaderRbsp->u32Len = encHeaderParam.size;
        pst_handle->header_encoded = 1;
    }

    vdi_free_dma_memory(pst_handle->core_idx, &vb_buffer, 0, 0);
    return ret;
}

static int cviVEncBuildEncParam(ENCODER_HANDLE *pst_handle, cviEncOnePicCfg *pPicCfg
                                , FrameBuffer *pst_fb, EncParam *pst_enc_param)
{
    PhysicalAddress *addr = NULL;
    PhysicalAddress *customMapAddr = NULL;
    cviEncParam *pCviEncParam = &pst_handle->cvi_enc_param;
    int src_idx;

    pst_fb->bufY = pPicCfg->phyAddrY;
    pst_fb->bufCb = pPicCfg->phyAddrCb;
    pst_fb->bufCr = pPicCfg->phyAddrCr;
    pst_fb->stride = pPicCfg->stride;
    pst_fb->cbcrInterleave = pPicCfg->cbcrInterleave;
    pst_fb->nv21 = pPicCfg->nv21;

    addr = Queue_Dequeue(pst_handle->free_stream_buffer);
    if (addr == NULL)
        return RETCODE_STREAM_BUF_FULL;

    if (pPicCfg->src_end == 0) {
        if (pPicCfg->src_idx < 0) {
            if (vb_phys_addr2handle(pst_fb->bufY) == VB_INVALID_HANDLE) {
                Queue_Enqueue(pst_handle->free_stream_buffer, addr);
                return RETCODE_INVALID_PARAM;
            }

            src_idx = get_frame_idx(pst_handle, pst_fb->bufY);
            if  (src_idx < 0) {
                Queue_Enqueue(pst_handle->free_stream_buffer, addr);
                VLOG(ERR, "Failed get_frame_idx idx:%d, bufY:0x%lx\n", src_idx, pst_fb->bufY);
                return RETCODE_FAILURE;
            }
        } else {
            src_idx = pPicCfg->src_idx;
        }
    } else {
        pst_handle->src_end = 1;
        src_idx = -1;
    }

    if (src_idx >= 0 && src_idx < MAX_SRC_BUFFER_NUM) {
        pst_handle->input_frame[src_idx].pts = pPicCfg->u64Pts;
        pst_handle->input_frame[src_idx].idx = pst_handle->frame_idx;
        pst_handle->input_frame[src_idx].custom_map_addr = pCviEncParam->customMapOpt.addrCustomMap;
    }

    pst_enc_param->sourceFrame = pst_fb;
    pst_enc_param->quantParam = 1;
    pst_enc_param->picStreamBufferAddr = *addr;
    pst_enc_param->picStreamBufferSize = pst_handle->open_param.bitstreamBufferSize;
    pst_enc_param->codeOption.implicitHeaderEncode = 1;
    pst_enc_param->srcIdx = src_idx;
    pst_enc_param->srcEndFlag = pPicCfg->src_end;
    pst_enc_param->pts = pPicCfg->u64Pts;
    if (pst_handle->virtualIPeriod > 0) {
        pst_enc_param->useCurSrcAsLongtermPic
            = (pst_handle->frame_idx % pst_handle->virtualIPeriod) == 0 ? 1 : 0;
        pst_enc_param->useLongtermRef
            = (pst_handle->frame_idx % pst_handle->virtualIPeriod) == 0 ? 1 : 0;
    }

    if (pCviEncParam->rcMode == RC_MODE_FIXQP) {
        pst_enc_param->forcePicQpEnable = 1;
        pst_enc_param->forcePicQpI = pCviEncParam->iqp;
        pst_enc_param->forcePicQpP = pCviEncParam->pqp;
        pst_enc_param->forcePicQpB = pCviEncParam->pqp;
    }

    { // if current frame have roi region
        int i;
        for (i = 0; i < MAX_NUM_ROI; i++) {
            if (pst_handle->roi_rect[i].roi_enable_flag == 1) {
                pCviEncParam->isCustomMapFlag = TRUE;
                break;
            }
        }
        if (pst_handle->last_frame_qp == 0) {  //for first frame, no last qp
            pst_handle->last_frame_qp = pst_handle->open_param.EncStdParam.waveParam.initialRcQp;
        }
    }
    if (src_idx >= 0 && src_idx < MAX_SRC_BUFFER_NUM) {
        if (pCviEncParam->isCustomMapFlag == TRUE && pCviEncParam->edge_api_enable == FALSE) {
            pCviEncParam->customMapOpt.customRoiMapEnable = true;
            customMapAddr = Queue_Dequeue(pst_handle->customMapBuffer);
            if (customMapAddr == NULL)
                return RETCODE_STREAM_BUF_FULL;

            setMapData(pst_handle->core_idx, pst_handle->roi_rect, pst_handle->last_frame_qp, pst_handle->open_param.picWidth, pst_handle->open_param.picHeight,
                                    &(pCviEncParam->customMapOpt), pst_handle->open_param.bitstreamFormat, customMapAddr);
        }
        pst_handle->input_frame[src_idx].pts = pPicCfg->u64Pts;
        pst_handle->input_frame[src_idx].idx = pst_handle->frame_idx;
        pst_handle->input_frame[src_idx].custom_map_addr = pCviEncParam->customMapOpt.addrCustomMap;
    }

    // update custom map
    if (pCviEncParam->isCustomMapFlag == TRUE) {
        // enc_param.customMapOpt.
        memcpy(&pst_enc_param->customMapOpt, &pCviEncParam->customMapOpt, sizeof(WaveCustomMapOpt));
        pCviEncParam->isCustomMapFlag = FALSE;
    }
    return 0;
}

int cviVEncEncOnePic(void *handle, cviEncOnePicCfg *pPicCfg, int s32MilliSec)
{
    int ret;
    EncHeaderParam encHeaderParam = {0};
    EncParam enc_param = {0};
    FrameBuffer src_buffer = {0};
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    PhysicalAddress *addr = NULL;
    QueueStatusInfo queue_status = {0};
    CodecInst *pCodecInst = NULL;
    BOOL is_wait_interrupt = 0;
    BOOL is_publish = 0;

    // create venc_wait thread
    // 1. bind_mode is false
    // 2. bind_mode is true and is_isolate_send is true
    if ((!pst_handle->is_bind_mode || (pst_handle->is_bind_mode && pst_handle->is_isolate_send))
        && (pst_handle->thread_handle == NULL || pst_handle->stop_thread == 1)) {
        // struct sched_param param = {
        //     .sched_priority = 95,
        // };
        VLOG(INFO, "cviVEncEncOnePic create venc wait thread, chn:%d\n", pst_handle->channel_index);

        pst_handle->stop_thread = 0;
        pst_handle->thread_handle = kthread_run(thread_wait_interrupt, pst_handle, "soph_vc_wait%d", pst_handle->channel_index);
        if (IS_ERR(pst_handle->thread_handle)) {
            pst_handle->thread_handle = NULL;
            return RETCODE_FAILURE;
        }
        // sched_setscheduler(pst_handle->thread_handle, SCHED_RR, &param);
    }

    pst_handle->chn_venc_info.sendAllYuvCnt++;
    VPU_EncGiveCommand(pst_handle->handle, ENC_GET_QUEUE_STATUS, &queue_status);
    if ((queue_status.instanceQueueCount == pst_handle->cmd_queue_depth) || (queue_status.instanceQueueFull == 1)) {
        VLOG(WARN, "vpu queue fail count:%d, queue full:%d\n"
            , queue_status.instanceQueueCount, queue_status.instanceQueueFull);
        pst_handle->cmd_queue_full = 1;
        return RETCODE_QUEUEING_FAILURE;
    }

    // update enc param
    pCodecInst = pst_handle->handle;
    pCodecInst->CodecInfo->encInfo.openParam.cbcrInterleave =
        pst_handle->open_param.cbcrInterleave;
    pCodecInst->CodecInfo->encInfo.openParam.nv21 = pst_handle->open_param.nv21;

    memset(&enc_param, 0, sizeof(EncParam));
    enc_param.is_idr_frame = cviCheckIdrPeriod(pst_handle);

    // for avbr bitrate estimate
    pst_open_param->picMotionLevel = pPicCfg->picMotionLevel;

    // check param change
    cviPicParamChangeCtrl(pst_handle, &enc_param);
    VLOG(INFO, "cviVEncEncOnePic chn:%d frameidx:%d idr:%d\n"
        , pst_handle->channel_index, pst_handle->frame_idx, enc_param.is_idr_frame);
    if (enc_param.is_idr_frame && !pst_handle->header_encoded) {
        // check cmd queue count and free_strem buffer cnt
        // if (queue_status.instanceQueueCount > COMMAND_QUEUE_DEPTH - 2
        //     || Queue_Get_Cnt(pst_handle->free_stream_buffer) < 2) {
        //     VLOG(INFO,"vpu encode idr frame cmdqueuecount:%d, bsqueue count:%d\n"
        //         , queue_status.instanceQueueCount, Queue_Get_Cnt(pst_handle->free_stream_buffer));
        //     return RETCODE_QUEUEING_FAILURE;
        // }
        addr = Queue_Dequeue(pst_handle->free_stream_buffer);
        if (addr == NULL) {
            VLOG(WARN, "build header free_stream_buffer is empty\n");
            return RETCODE_STREAM_BUF_FULL;
        }

        if (!pst_handle->thread_handle) {
            is_wait_interrupt = 1;
            is_publish = 1;
        }
        VLOG(INFO, "build header wait:%d publish:%d\n", is_wait_interrupt, is_publish);
        osal_memset(&encHeaderParam, 0x00, sizeof(EncHeaderParam));
        encHeaderParam.buf = *addr;
        encHeaderParam.size = pst_handle->open_param.bitstreamBufferSize;
        ret = cviBuildEncodeHeader(pst_handle, &encHeaderParam, is_wait_interrupt, is_publish);
        if (ret != RETCODE_SUCCESS) {
            Queue_Enqueue(pst_handle->free_stream_buffer, addr);
            VLOG(ERR, "Failed ENC_PUT_VIDEO_HEADER(ret:%d)\n", ret);
            return ret;
        }
    }

    ret = cviVEncBuildEncParam(pst_handle, pPicCfg, &src_buffer, &enc_param);
    if (ret == RETCODE_STREAM_BUF_FULL) {
        VLOG(WARN, "cviVEncBuildEncParam streambuf not enough, ret:%d, remain packs:%d\n"
            , ret, Queue_Get_Cnt(pst_handle->stream_packs));
        return ret;
    } else if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "Failed cviVEncBuildEncParam(ret:%d)\n", ret);
        return ret;
    }

    if (Queue_Get_Cnt(pst_handle->free_stream_buffer) <= 1) {
        pst_handle->cmd_queue_full = 1;
    }

    ret = VPU_EncStartOneFrame(pst_handle->handle, &enc_param);
    if (ret == RETCODE_QUEUEING_FAILURE) {
        osal_msleep(1);
        ret = VPU_EncStartOneFrame(pst_handle->handle, &enc_param);
    }
    if (ret != RETCODE_SUCCESS) {
        release_frame_idx(pst_handle, enc_param.srcIdx);
        Queue_Enqueue(pst_handle->free_stream_buffer, &enc_param.picStreamBufferAddr);
        if (ret == RETCODE_QUEUEING_FAILURE) {
            VLOG(WARN, "VPU_EncStartOneFrame queue fail (ret:%d), chn:%d\n", ret, pst_handle->channel_index);
        } else {
            VLOG(ERR, "Failed VPU_EncStartOneFrame(ret:%d), chn:%d\n", ret, pst_handle->channel_index);
        }
        return ret;
    }

    pst_handle->chn_venc_info.sendOkYuvCnt++;
    if (pst_handle->frame_idx == 0) {
        pst_handle->first_frame_pts = enc_param.pts;
    }

    memset(&queue_status, 0, sizeof(QueueStatusInfo));
    VPU_EncGiveCommand(pst_handle->handle, ENC_GET_QUEUE_STATUS, &queue_status);
    if ((queue_status.instanceQueueCount == pst_handle->cmd_queue_depth) || (queue_status.instanceQueueFull == 1)) {
        pst_handle->cmd_queue_full = 1;
    }

    pst_handle->frame_idx++;
    // set_vb_flag(pPicCfg->phyAddrY);

    if (pst_handle->src_end && pst_handle->ouput_end == 0) {
        VLOG(WARN, "vpu queue fail for wait output end \n");
        return RETCODE_QUEUEING_FAILURE;
    }

    return 0;
}

static int cviGetEncodedInfo(void *handle, int s32MilliSec)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncInitialInfo init_info = {0};
    EncOutputInfo output_info;
    int int_reason = 0;
    int retry_times = 0;
    int ret = 0;
    int cyclePerTick = 256;
    stPack encode_pack = {0};

    do {
        int_reason = VPU_WaitInterruptEx(pst_handle->handle, s32MilliSec);
        if (int_reason == -1) {
            if (s32MilliSec >= 0) {
                VLOG(WARN, "Error : encoder timeout happened in non_block mode \n");
                return -2;
            }
        }

        if (int_reason > 0) {
            VPU_ClearInterruptEx(pst_handle->handle, int_reason);

            if (int_reason & (1 << INT_WAVE5_ENC_SET_PARAM)) {
                ret = VPU_EncCompleteSeqInit(pst_handle->handle, &init_info);
                if (ret == RETCODE_VPU_RESPONSE_TIMEOUT) {
                    VLOG(ERR, "<%s:%d> Failed to VPU_EncCompleteSeqInit()\n", __FUNCTION__, __LINE__);
                    return -1;
                }
                continue;
            }

            if (int_reason & (1 << INT_WAVE5_ENC_PIC)) {
                memset(&output_info, 0, sizeof(EncOutputInfo));
                do {
                    ret = VPU_EncGetOutputInfo(pst_handle->handle, &output_info);
                    if (ret != RETCODE_SUCCESS) {
                        VLOG(TRACE, "VPU_EncGetOutputInfo ret:%d, pictype:0x%x, streamSize:%d\n"
                            , ret, output_info.picType, output_info.bitstreamSize);
                        retry_times++;
                        osal_msleep(1);
                    } else {
                        break;
                    }
                } while (retry_times <= 5);

                if (ret != RETCODE_SUCCESS) {
                    VLOG(ERR, "Failed VPU_EncGetOutputInfo ret:%d, reason:0x%x\n", ret, output_info.errorReason);
                    return -1;
                }

                if (output_info.bitstreamSize > 0) {
                    if (output_info.picType == PIC_TYPE_I || output_info.picType == PIC_TYPE_IDR) {
                        if (pst_handle->backupPack.len > 0) {
                            // header and idr need 2 packs
                            if (Queue_Get_Cnt(pst_handle->stream_packs) < (MAX_NUM_PACKS-1)) {
                                memcpy(&encode_pack, &pst_handle->backupPack, sizeof(stPack));
                                memset(&pst_handle->backupPack, 0, sizeof(stPack));
                                Queue_Enqueue(pst_handle->stream_packs, &encode_pack);
                            }
                        }
                    }

                    if (output_info.avgCtuQp > 0) {
                        ret = cviInsertUserData(pst_handle);
                        if (ret == FALSE) {
                            VLOG(ERR, "cviInsertUserData failed %d\n", ret);
                        }
                    }

                    vdi_invalidate_ion_cache(output_info.bitstreamBuffer
                                            , phys_to_virt(output_info.bitstreamBuffer)
                                            , output_info.bitstreamSize);
                    // drop this packet
                    if (Queue_Get_Cnt(pst_handle->stream_packs) >= MAX_NUM_PACKS) {
                        pst_handle->chn_venc_info.dropCnt++;
                        Queue_Enqueue(pst_handle->free_stream_buffer, &output_info.bitstreamBuffer);
                        VLOG(ERR, "bitstream queue is full!\n");
                        return -1;
                    }

                    encode_pack.u64PhyAddr = output_info.bitstreamBuffer;
                    encode_pack.addr = phys_to_virt(output_info.bitstreamBuffer);
                    encode_pack.len = output_info.bitstreamSize;
                    encode_pack.encSrcIdx = output_info.encSrcIdx;
                    encode_pack.cviNalType = output_info.picType;
                    encode_pack.need_free = FALSE;
                    encode_pack.u64PTS = output_info.pts;
                    encode_pack.u64DTS =
                            output_info.encPicCnt - 1 - pst_handle->bframe_delay + pst_handle->first_frame_pts;
                    encode_pack.u32AvgCtuQp = output_info.avgCtuQp;
                    encode_pack.bUsed = FALSE;
                    encode_pack.u32EncHwTime =
                            (output_info.encEncodeEndTick - output_info.encHostCmdTick)*cyclePerTick/(VPU_STAT_CYCLES_CLK/1000000);
                    if (output_info.encSrcIdx >= 0 && output_info.encSrcIdx < MAX_SRC_BUFFER_NUM) {
                        encode_pack.u64CustomMapAddr = pst_handle->input_frame[output_info.encSrcIdx].custom_map_addr;
                    }
                    Queue_Enqueue(pst_handle->stream_packs, &encode_pack);
                    Queue_Enqueue(pst_handle->customMapBuffer, &pst_handle->input_frame[output_info.encSrcIdx].custom_map_addr);
                }

                if (output_info.encSrcIdx >= 0 && output_info.encSrcIdx < MAX_SRC_BUFFER_NUM) {
                    release_frame_idx(pst_handle, output_info.encSrcIdx);
                }

                if (pst_handle->src_end == 1 && output_info.reconFrameIndex == RECON_IDX_FLAG_ENC_END) {
                    pst_handle->ouput_end = 1;
                }

                pst_handle->chn_venc_info.getStreamCnt++;
                wake_up(&tVencWaitQueue[pst_handle->channel_index]);
                return 0;
            }

            if (ret & (1 << INT_WAVE5_BSBUF_FULL)) {
            }
        }
    } while (int_reason == INTERRUPT_TIMEOUT_VALUE || int_reason == 0 || int_reason == 512);

    VLOG(ERR, "impossible here, chn:%d int reson:%d\n", pst_handle->channel_index, int_reason);
    return -1;
}

int cviVEncGetStream(void *handle, cviVEncStreamInfo *pStreamInfo, int s32MilliSec)
{
    ENCODER_HANDLE *pst_handle = handle;
    int ret = RETCODE_SUCCESS;
    uint64_t lastGetTime = 0;

    if (!pst_handle->is_bind_mode || pst_handle->is_isolate_send) {
        if (s32MilliSec > 0) {
            lastGetTime = osal_gettime();
        REWAIT:
            ret = wait_for_completion_timeout(&pst_handle->semGetStreamCmd,
                    usecs_to_jiffies(s32MilliSec * 1000));
            if (ret == 0) {
                if (Queue_Get_Cnt(pst_handle->stream_packs) > 0) {
                    pStreamInfo->psp = pst_handle->stream_packs;
                    return 0;
                }
                return -2;
            } else if (Queue_Get_Cnt(pst_handle->stream_packs) == 0) {
                if (osal_gettime() >= (lastGetTime + s32MilliSec)) {
                    return -2;
                }

                // wait again
                goto REWAIT;
            }
        }
    } else if (pst_handle->is_bind_mode) {
        ret = cviGetEncodedInfo(handle, s32MilliSec);
        if (ret != RETCODE_SUCCESS) {
            VLOG(ERR, "cviGetEncodedInfo ret %d\n", ret);
            return ret;
        }
    }
    VLOG(INFO, "cviVEncGetStream chn:%d iso:%d, ret:%d\n"
            , pst_handle->channel_index, pst_handle->is_isolate_send, ret);

    if (Queue_Get_Cnt(pst_handle->stream_packs) == 0) {
        if (pst_handle->ouput_end == 1) {
            return 0;
        }
        return -2;
    }
    pStreamInfo->psp = pst_handle->stream_packs;
    return 0;
}

int cviVEncReleaseStream(void *handle, stPack *pstPack, unsigned int packCnt)
{
    int i = 0;
    ENCODER_HANDLE *pst_handle = handle;

    if (!pstPack || packCnt == 0)
        return 0;

    for (i=0; i < packCnt; i++) {
        if (pstPack[i].cviNalType == NAL_SEI) {
            osal_kfree(pstPack[i].addr);
        } else {
            Queue_Enqueue(pst_handle->free_stream_buffer, &pstPack[i].u64PhyAddr);
        }
    }

    return 0;
}

int cviVEncSetRc(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    cviRcParam *prcp = (cviRcParam *)arg;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    int ret = 0;

    pst_open_param->rcIntervalMode = prcp->u32RowQpDelta;
    pst_open_param->rcInitDelay = prcp->s32InitialDelay;

    pst_open_param->maxIprop = prcp->u32MaxIprop;
    pst_open_param->minIprop = prcp->u32MinIprop;
    pst_open_param->avbrFrmLostOpen = prcp->s32AvbrFrmLostOpen;
    pst_open_param->avbrFrmGaps = prcp->s32AvbrFrmGap;
    pst_open_param->maxStillQp = prcp->u32MaxStillQP;
    pst_open_param->motionSensitivy = prcp->u32MotionSensitivity;
    pst_open_param->pureStillThr = prcp->s32AvbrPureStillThr;
    pst_open_param->minStillPercent = prcp->s32MinStillPercent;
    pst_open_param->changePos = prcp->s32ChangePos;

    param->initialRcQp = prcp->firstFrmstartQp;
    param->hvsQpScale =
        (prcp->u32ThrdLv <= 4) ? (int)prcp->u32ThrdLv : 2;

    if (pst_open_param->bitstreamFormat == STD_HEVC) {
        param->cuLevelRCEnable = (prcp->u32RowQpDelta > 0);
    } else {
        param->mbLevelRcEnable = (prcp->u32RowQpDelta > 0);
    }

    param->maxQpI = prcp->u32MaxIQp;
    param->minQpI = prcp->u32MinIQp;
    param->maxQpP = prcp->u32MaxQp;
    param->minQpP = prcp->u32MinQp;
    param->maxQpB = prcp->u32MaxQp;
    param->minQpB = prcp->u32MinQp;

    return ret;
}

int cviVEncStart(void *handle, void *arg)
{
    int ret;
    ENCODER_HANDLE *pst_handle = handle;
    EncInitialInfo init_info = {0};
    SecAxiUse secAxiUse = {0};
    int cyclePerTick = 32768;
    VpuAttr product_attr;
    vpu_buffer_t vb_buffer;
    int i = 0;
    int chn = *(int *)arg;
    int int_reason = 0;

    ret = VPU_GetProductInfo(pst_handle->core_idx, &product_attr);
    if (ret != RETCODE_SUCCESS) {
        return ret;
    }

    ret = VPU_EncOpen(&pst_handle->handle, &pst_handle->open_param);
    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "Failed to VPU_EncOpen(ret:%d)\n", ret);
        return ret;
    }
    pst_handle->channel_index = chn;

    // rotation and mirror direction
    if (pst_handle->cvi_enc_param.rotationAngle) {
        VPU_EncGiveCommand(pst_handle->handle, ENABLE_ROTATION, 0);
        VPU_EncGiveCommand(pst_handle->handle, SET_ROTATION_ANGLE, &pst_handle->cvi_enc_param.rotationAngle);

    }

    if (pst_handle->cvi_enc_param.mirrorDirection) {
        VPU_EncGiveCommand(pst_handle->handle, ENABLE_MIRRORING, 0);
        VPU_EncGiveCommand(pst_handle->handle, SET_MIRROR_DIRECTION, &pst_handle->cvi_enc_param.mirrorDirection);
    }

    // when picWidth less than 4608, support secAXI
    if (pst_handle->open_param.picWidth <= 4608) {
        secAxiUse.u.wave.useEncRdoEnable = TRUE;
        secAxiUse.u.wave.useEncLfEnable  = TRUE;
    } else {
        secAxiUse.u.wave.useEncRdoEnable = FALSE;
        secAxiUse.u.wave.useEncLfEnable  = FALSE;
    }

    VPU_EncGiveCommand(pst_handle->handle, SET_SEC_AXI, &secAxiUse);

    if (product_attr.supportNewTimer == TRUE)
        cyclePerTick = 256;
    VPU_EncGiveCommand(pst_handle->handle, SET_CYCLE_PER_TICK, (void *)&cyclePerTick);

    do {
        ret = VPU_EncIssueSeqInit(pst_handle->handle);
    } while (ret == RETCODE_QUEUEING_FAILURE);

    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "<%s:%d> Failed to VPU_EncIssueSeqInit() ret(%d)\n"
            , __FUNCTION__, __LINE__, ret);
        return ret;
    }

    do {
        int_reason = VPU_WaitInterruptEx(pst_handle->handle, 1000);
    } while (int_reason == INTERRUPT_TIMEOUT_VALUE);

    if (int_reason < 0) {
        VLOG(ERR, "<%s:%d> Failed to VPU_WaitInterruptEx int_reason(%d)\n"
            , __FUNCTION__, __LINE__, int_reason);
        goto ERR_VPU_ENC_OPEN;
    }

    if (int_reason > 0)
        VPU_ClearInterruptEx(pst_handle->handle, int_reason);

    if (int_reason & (1 << INT_WAVE5_ENC_SET_PARAM)) {
        ret = VPU_EncCompleteSeqInit(pst_handle->handle, &init_info);
        if (ret == RETCODE_VPU_RESPONSE_TIMEOUT) {
            VLOG(ERR, "<%s:%d> Failed to VPU_EncCompleteSeqInit()\n", __FUNCTION__, __LINE__);
            goto ERR_VPU_ENC_OPEN;
        }

        pst_handle->min_recon_frame_count = init_info.minFrameBufferCount;
        pst_handle->min_src_frame_count = init_info.minSrcFrameCount + pst_handle->cmd_queue_depth;

        ret = alloc_framebuffer(pst_handle);
        if ( ret != RETCODE_SUCCESS) {
            VLOG(ERR, "<%s:%d> Failed to alloc_framebuffer\n", __FUNCTION__, __LINE__);
            goto ERR_VPU_ENC_OPEN;
        }

        // additional is for store header and header_backup
        pst_handle->free_stream_buffer = Queue_Create_With_Lock(pst_handle->min_src_frame_count + EXTERN_SRC_BUFFER_CNT, sizeof(PhysicalAddress));

        memset(&vb_buffer, 0, sizeof(vpu_buffer_t));
        vb_buffer.size = pst_handle->open_param.bitstreamBufferSize;
        for (i = 0; i < pst_handle->min_src_frame_count + EXTERN_SRC_BUFFER_CNT; i++) {
            ret = vdi_allocate_dma_memory(pst_handle->core_idx, &vb_buffer, 0, 0);
            if (ret != RETCODE_SUCCESS) {
                VLOG(ERR, "<%s:%d> Failed to alloc bitstream_buffer\n", __FUNCTION__, __LINE__);
                goto ERR_VPU_ENC_OPEN;
            }
            pst_handle->bitstream_buffer[i] = vb_buffer.phys_addr;
            Queue_Enqueue(pst_handle->free_stream_buffer, &vb_buffer.phys_addr);
        }

        pst_handle->open_param.bitstreamBuffer = pst_handle->bitstream_buffer[0];

        if (pst_handle->enable_cvi_rc == 1) {
            pst_handle->open_param.cviRcEn = 1;
            cviEncRc_Open(&pst_handle->rc_info, &pst_handle->open_param);
        }
        return RETCODE_SUCCESS;
    }

ERR_VPU_ENC_OPEN:
    VPU_EncClose(pst_handle->handle);
    return RETCODE_FAILURE;
}

int cviVEncOpGetFd(void *handle, void *arg)
{
    return 0;
}

int cviVEncSetRequestIDR(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;

    pst_handle->cvi_enc_param.idr_request = TRUE;
    return 0;
}

int cviVEncSetEnableIDR(void *handle, void *arg)
{
    BOOL bEnable = *(CVI_BOOL *)arg;
    ENCODER_HANDLE *pst_handle = handle;
    int period;

    if (bEnable == TRUE) {
        if (pst_handle->cvi_enc_param.enable_idr == FALSE) {
            // next frame set to idr
            pst_handle->cvi_enc_param.enable_idr = TRUE;
            pst_handle->cvi_enc_param.idr_request = TRUE;
            // restore old period
            period = -1;
            VPU_EncGiveCommand(pst_handle->handle, ENC_SET_PERIOD, &period);
        }
    } else {
          period = 0;
          pst_handle->cvi_enc_param.enable_idr = FALSE;
          pst_handle->cvi_enc_param.idr_request = FALSE;
          VPU_EncGiveCommand(pst_handle->handle, ENC_SET_PERIOD, &period);
    }

    return 0;
}

int cviVEncOpSetChnAttr(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    cviVidChnAttr *pChnAttr = (cviVidChnAttr *)arg;
    unsigned int u32Sec = 0;
    unsigned int u32Frm = 0;

    pst_open_param->bitRate = pChnAttr->u32BitRate*1024;

    u32Sec = pChnAttr->fr32DstFrameRate >> 16;
    u32Frm = pChnAttr->fr32DstFrameRate & 0xFFFF;

    if (u32Sec == 0) {
        pst_open_param->frameRateInfo = u32Frm;
    } else {
        pst_open_param->frameRateInfo = ((u32Sec - 1) << 16) + u32Frm;
    }

    return 0;
}

int cviVEncSetRef(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    unsigned int *tempLayer = (unsigned int *)arg;

    if (*tempLayer == 2) {
        param->gopPresetIdx = 0;
        param->gopParam.customGopSize = 2;

        param->gopParam.picParam[0].picType = PIC_TYPE_P;
        param->gopParam.picParam[0].pocOffset = 1;
        param->gopParam.picParam[0].picQp = 26;
        param->gopParam.picParam[0].refPocL0 = 0;
        param->gopParam.picParam[0].refPocL1 = 0;
        param->gopParam.picParam[0].temporalId  = 1;

        param->gopParam.picParam[1].picType = PIC_TYPE_P;
        param->gopParam.picParam[1].pocOffset = 2;
        param->gopParam.picParam[1].picQp = 28;
        param->gopParam.picParam[1].refPocL0 = 0;
        param->gopParam.picParam[1].refPocL1 = 0;
        param->gopParam.picParam[1].temporalId  = 0;
    } else if (*tempLayer == 3) {
        param->gopPresetIdx = 0;
        param->gopParam.customGopSize = 4;

        param->gopParam.picParam[0].picType = PIC_TYPE_P;
        param->gopParam.picParam[0].pocOffset = 1;
        param->gopParam.picParam[0].picQp = 30;
        param->gopParam.picParam[0].refPocL0 = 0;
        param->gopParam.picParam[0].refPocL1 = 0;
        param->gopParam.picParam[0].temporalId  = 2;

        param->gopParam.picParam[1].picType = PIC_TYPE_P;
        param->gopParam.picParam[1].pocOffset = 2;
        param->gopParam.picParam[1].picQp = 28;
        param->gopParam.picParam[1].refPocL0 = 0;
        param->gopParam.picParam[1].refPocL1 = 0;
        param->gopParam.picParam[1].temporalId  = 1;

        param->gopParam.picParam[2].picType = PIC_TYPE_P;
        param->gopParam.picParam[2].pocOffset = 3;
        param->gopParam.picParam[2].picQp = 30;
        param->gopParam.picParam[2].refPocL0 = 2;
        param->gopParam.picParam[2].refPocL1 = 0;
        param->gopParam.picParam[2].temporalId  = 2;

        param->gopParam.picParam[3].picType = PIC_TYPE_P;
        param->gopParam.picParam[3].pocOffset = 4;
        param->gopParam.picParam[3].picQp = 26;
        param->gopParam.picParam[3].refPocL0 = 0;
        param->gopParam.picParam[3].refPocL1 = 0;
        param->gopParam.picParam[3].temporalId  = 0;
    }

    return 0;
}

int cviVEncSetRoi(void *handle, void *arg)
{
    cviRoiParam *roiParam = (cviRoiParam *)arg;
    ENCODER_HANDLE *pst_handle = (ENCODER_HANDLE *)handle;
    int idx;
    vpu_buffer_t vb_buffer;
    int index = roiParam->roi_index;
    int picWidth = pst_handle->open_param.picWidth;
    int picHeight = pst_handle->open_param.picHeight;
    int MbWidth  =  VPU_ALIGN16(picWidth) >> 4;
    int MbHeight =  VPU_ALIGN16(picHeight) >> 4;

    int ctuMapWidthCnt  = VPU_ALIGN64(picWidth) >> 6;
    int ctuMapHeightCnt = VPU_ALIGN64(picHeight) >> 6;

    int MB_NUM = MbWidth * MbHeight;
    int CTU_NUM = ctuMapWidthCnt * ctuMapHeightCnt ;


    // when first set roi,you need to alloc memory
    if (pst_handle->customMapBuffer == NULL) {
        pst_handle->customMapBuffer = Queue_Create_With_Lock(pst_handle->min_src_frame_count, sizeof(PhysicalAddress));

        memset(&vb_buffer, 0, sizeof(vpu_buffer_t));
        pst_handle->customMapBufferSize = (pst_handle->open_param.bitstreamFormat == STD_AVC) ? MB_NUM : CTU_NUM * 8;
        vb_buffer.size = pst_handle->customMapBufferSize;

        for (idx = 0; idx < pst_handle->min_src_frame_count; idx++) {
            if (vdi_allocate_dma_memory(pst_handle->core_idx, &vb_buffer, ENC_ETC, 0) < 0) {
                VLOG(ERR,"vdi_allocate_dma_memory vbCustomMap failed\n",__func__,__LINE__);
                return RETCODE_FAILURE;
            }
            Queue_Enqueue(pst_handle->customMapBuffer, &vb_buffer.phys_addr);
        }
    }
    if (index < 0 || index > MAX_NUM_ROI) {
        VLOG(ERR, "<%s:%d> roi index is invalid\n", __FUNCTION__, __LINE__);
        return -1;
    }
    memcpy(&pst_handle->roi_rect[index],roiParam,sizeof(cviRoiParam));
    return 0;
}

int cviVEncGetRoi(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    cviRoiParam *roiParam = (cviRoiParam *)arg;

    int index = roiParam->roi_index;
    const cviRoiParam *pRoiParam = NULL;

    if (index > MAX_NUM_ROI || index < 0) {
        VLOG(ERR, "<%s:%d> roi index is invalid\n", __FUNCTION__, __LINE__);
        return -1;
    }

    pRoiParam = &pst_handle->roi_rect[index];

    roiParam->roi_enable_flag = pRoiParam->roi_enable_flag;
    roiParam->is_absolute_qp = pRoiParam->is_absolute_qp;
    roiParam->roi_qp = pRoiParam->roi_qp;
    roiParam->roi_rect_x = pRoiParam->roi_rect_x;
    roiParam->roi_rect_y = pRoiParam->roi_rect_y;
    roiParam->roi_rect_width = pRoiParam->roi_rect_width;
    roiParam->roi_rect_height = pRoiParam->roi_rect_height;
    return 0;
}

int cviVEncSetFrameLost(void *handle, void *arg)
{
    return 0;
}

int cviVEncGetVbInfo(void *handle, void *arg)
{
    return 0;
}

int cviVEncSetVbBuffer(void *handle, void *arg)
{
    return 0;
}

int cviVEncEncodeUserData(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    cviUserData *pSrc = (cviUserData *)arg;
    unsigned int len;
    UserDataList *userdataNode = NULL;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    cviEncParam *pCviEncParam = &pst_handle->cvi_enc_param;

    if (pSrc == NULL || pSrc->userData == NULL || pSrc->len == 0) {
        VLOG(ERR, "no user data\n");
        return -1;
    }

    userdataNode = (UserDataList *)osal_calloc(1, sizeof(UserDataList));
    if (userdataNode == NULL)
        return -1;

    userdataNode->userDataBuf = (Uint8 *)osal_calloc(1, pCviEncParam->userDataBufSize);
    if (userdataNode->userDataBuf == NULL) {
        osal_free(userdataNode);
        return -1;
    }

    len = seiEncode(pst_open_param->bitstreamFormat, pSrc->userData,
              pSrc->len, userdataNode->userDataBuf, pCviEncParam->userDataBufSize);

    if (len > pCviEncParam->userDataBufSize) {
        VLOG(ERR, "encoded user data len %d exceeds buffer size %d\n",
            len, pCviEncParam->userDataBufSize);
        osal_free(userdataNode);
        osal_free(userdataNode->userDataBuf);
        return -1;
    }

    userdataNode->userDataLen = len;
    list_add_tail(&userdataNode->list, &pCviEncParam->userdataList);

    return 0;
}

int cviVEncSetPred(void *handle, void *arg)
{
    // wave521 not support
    return 0;
}

int cviVEncSetH264Entropy(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    cviH264Entropy *pSrc = (cviH264Entropy *)arg;
    int h264EntropyMode = 0;

    if (pSrc == NULL) {
        VLOG(ERR, "no h264 entropy data\n");
        return -1;
    }

    if (pSrc->entropyEncModeI == 0 && pSrc->entropyEncModeP == 0)
        h264EntropyMode = 0;
    else
        h264EntropyMode = 1;

    param->entropyCodingMode = h264EntropyMode;
    return 0;
}

int cviVEncSetH265PredUnit(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    cviH265Pu *pSrc = (cviH265Pu *)arg;

    if (pSrc == NULL) {
        VLOG(ERR, "no h265 PredUnit data\n");
        return -1;
    }

    param->constIntraPredFlag = pSrc->constrained_intra_pred_flag;
    param->strongIntraSmoothEnable = pSrc->strong_intra_smoothing_enabled_flag;

    return 0;
}

int cviVEncGetH265PredUnit(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    cviH265Pu *pDst = (cviH265Pu *)arg;

    if (pDst == NULL) {
        VLOG(ERR, "no h265 PredUnit data\n");
        return -1;
    }

    pDst->constrained_intra_pred_flag = param->constIntraPredFlag;
    pDst->strong_intra_smoothing_enabled_flag = param->strongIntraSmoothEnable;

    return 0;
}

int cviVEncSetSearchWindow(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    cviVencSearchWindow *pSrc = (cviVencSearchWindow *)arg;

    if (pSrc == NULL) {
        VLOG(ERR, "no search window data\n");
        return -1;
    }

    if (pSrc->u32Ver < 4 || pSrc->u32Ver > 32) {
        VLOG(ERR, "search Ver should be [4,32]\n");
        return -1;
    }

    if (pSrc->u32Hor < 4 || pSrc->u32Hor > 16) {
        VLOG(ERR, "search Hor should be [4,16]\n");
        return -1;
    }

    param->s2SearchRangeXDiv4 = pSrc->u32Ver;
    param->s2SearchRangeYDiv4 = pSrc->u32Hor;

    return 0;
}

int cviVEncGetSearchWindow(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    cviVencSearchWindow *pDst = (cviVencSearchWindow *)arg;

    if (pDst == NULL) {
        VLOG(ERR, "no search window data\n");
        return -1;
    }
    //only support manual mode
    pDst->mode = search_mode_manual;
    pDst->u32Ver = param->s2SearchRangeXDiv4;
    pDst->u32Hor = param->s2SearchRangeYDiv4;

    return 0;
}


int cviVEncSetH264Trans(void *handle, void *arg)
{
    return 0;
}

int cviVEncSetH265Trans(void *handle, void *arg)
{
    return 0;
}

int cviVEncRegReconBuf(void *handle, void *arg)
{
    return 0;
}

int cviVEncSetInPixelFormat(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    cviInPixelFormat *pInFormat = (cviInPixelFormat *)arg;
    EncOpenParam *pst_open_param = &pst_handle->open_param;

    pst_open_param->cbcrInterleave = pInFormat->bCbCrInterleave;
    pst_open_param->nv21 = pInFormat->bNV21;

    return 0;
}

int cviVEncShowChnInfo(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    struct seq_file *m = (struct seq_file *)arg;

    if (pst_handle) {
        seq_printf(m, "chn num:%d\t sendCnt:%u\t sendOkCnt:%u\t getCnt:%u\t dropCnt:%u\n"
            , pst_handle->channel_index, pst_handle->chn_venc_info.sendAllYuvCnt
            , pst_handle->chn_venc_info.sendOkYuvCnt, pst_handle->chn_venc_info.getStreamCnt
            , pst_handle->chn_venc_info.dropCnt);
    }

    return 0;
}

int cviVEncSetUserRcInfo(void *handle, void *arg)
{
    return 0;
}

int cviVEncSetSuperFrame(void *handle, void *arg)
{
    return 0;
}

int cviVEncSetH264Vui(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    cviH264Vui *pSrc = (cviH264Vui *)arg;

    if (pSrc == NULL) {
        VLOG(ERR, "no h264 vui data\n");
        return -1;
    }

    cviH264SpsAddVui(pst_handle, pSrc);
    return 0;
}

int cviVEncSetH265Vui(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    cviH265Vui *pSrc = (cviH265Vui *)arg;

    if (pSrc == NULL) {
        VLOG(ERR, "no h265 vui data\n");
        return -1;
    }

    cviH265SpsAddVui(pst_handle, pSrc);
    return 0;
}

int cviVEncSetFrameParam(void *handle, void *arg)
{
    return 0;
}

static int cviVEncWaitEncodeDone(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    int ret = RETCODE_SUCCESS;

    ret = wait_for_completion_timeout(&pst_handle->semEncDoneCmd,
                usecs_to_jiffies(2000 * 1000));
    if (ret == 0) {
        VLOG(ERR, "get stream timeout!\n");
        return -1;
    }

    return 0;
}

int cviVEncSetH264Split(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    cviH264Split *pSrc = (cviH264Split *)arg;

    param->avcSliceMode = pSrc->bSplitEnable;
    param->avcSliceArg = pSrc->u32MbLineNum * (pst_open_param->picWidth + 15) / 16;
    return 0;
}

int cviVEncSetH265Split(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    cviH265Split *pSrc = (cviH265Split *)arg;

    param->independSliceMode = pSrc->bSplitEnable;
    param->independSliceModeArg = pSrc->u32LcuLineNum * (pst_open_param->picWidth + 63) / 64;
    return 0;
}

int cviVEncSetH264DBlk(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    cviH264Dblk *pSrc = (cviH264Dblk *)arg;

    param->disableDeblk = pSrc->disable_deblocking_filter_idc;
    param->lfCrossSliceBoundaryEnable = 1;
    param->betaOffsetDiv2 = pSrc->slice_beta_offset_div2;
    param->tcOffsetDiv2 = pSrc->slice_alpha_c0_offset_div2;

    return 0;
}

int cviVEncSetH265Dblk(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    cviH265Dblk *pSrc = (cviH265Dblk *)arg;

    param->disableDeblk = pSrc->slice_deblocking_filter_disabled_flag;
    param->lfCrossSliceBoundaryEnable = 1;
    param->betaOffsetDiv2 = pSrc->slice_beta_offset_div2;
    param->tcOffsetDiv2 = pSrc->slice_tc_offset_div2;

    return 0;
}

int cviVEncSetH264IntraPred(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    cviH264IntraPred *pSrc = (cviH264IntraPred *)arg;

    param->constIntraPredFlag = pSrc->constrained_intra_pred_flag;
    return 0;
}

int cviVEncSetCustomMap(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    WaveCustomMapOpt *pCustomOpt = (WaveCustomMapOpt *)arg;
    cviEncParam *pCviEncParam = &pst_handle->cvi_enc_param;

    memcpy(&pCviEncParam->customMapOpt, pCustomOpt, sizeof(WaveCustomMapOpt));
    pCviEncParam->isCustomMapFlag = TRUE;
    pCviEncParam->edge_api_enable = TRUE;      //when use cviVEncSetCustomMap, must set edge_api_enable is true, and cviVEncSetRoi is bypassed
    return 0;
}

int cviVEncGetInitialInfo(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    cviVencIntialInfo *pEncInitialInfo = (cviVencIntialInfo *)arg;

    pEncInitialInfo->min_num_rec_fb = pst_handle->min_recon_frame_count;
    pEncInitialInfo->min_num_src_fb = pst_handle->min_src_frame_count;

    return 0;
}

int cviVEncSetH265Sao(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    EncOpenParam *pst_open_param = &pst_handle->open_param;
    EncWave5Param *param = &pst_open_param->EncStdParam.waveParam;
    cviH265Sao *pSrc = (cviH265Sao *)arg;
    int h265SaoEnable = 0;

    if (pSrc == NULL) {
        VLOG(ERR, "no h265 sao data\n");
        return -1;
    }

    if (pSrc->slice_sao_luma_flag == 0 && pSrc->slice_sao_chroma_flag == 0)
        h265SaoEnable = 0;
    else
        h265SaoEnable = 1;

    param->saoEnable = h265SaoEnable;
    return 0;
}

int cviVEncSetBindMode(void *handle, void *arg)
{
    ENCODER_HANDLE *pst_handle = handle;
    int *pSrc = (int *)arg;

    if (pSrc == NULL) {
        VLOG(ERR, "invalid param\n");
        return -1;
    }

    pst_handle->is_bind_mode = *pSrc;
    VLOG(INFO, "cviVEncSetBindMode mode:%d\n", pst_handle->is_bind_mode);
    return 0;
}

typedef struct _CVI_VENC_IOCTL_OP_ {
    int opNum;
    int (*ioctlFunc)(void *handle, void *arg);
} CVI_VENC_IOCTL_OP;

CVI_VENC_IOCTL_OP cviIoctlOp[] = {
    { CVI_H26X_OP_NONE, NULL },
    { CVI_H26X_OP_SET_RC_PARAM, cviVEncSetRc },
    { CVI_H26X_OP_START, cviVEncStart },
    { CVI_H26X_OP_GET_FD, cviVEncOpGetFd },
    { CVI_H26X_OP_SET_REQUEST_IDR, cviVEncSetRequestIDR },
    { CVI_H26X_OP_SET_ENABLE_IDR, cviVEncSetEnableIDR},
    { CVI_H26X_OP_SET_CHN_ATTR, cviVEncOpSetChnAttr },
    { CVI_H26X_OP_SET_REF_PARAM, cviVEncSetRef },
    { CVI_H26X_OP_SET_ROI_PARAM, cviVEncSetRoi },
    { CVI_H26X_OP_GET_ROI_PARAM, cviVEncGetRoi },
    { CVI_H26X_OP_SET_FRAME_LOST_STRATEGY, cviVEncSetFrameLost },
    { CVI_H26X_OP_GET_VB_INFO, cviVEncGetVbInfo },
    { CVI_H26X_OP_SET_VB_BUFFER, cviVEncSetVbBuffer },
    { CVI_H26X_OP_SET_USER_DATA, cviVEncEncodeUserData },
    { CVI_H26X_OP_SET_PREDICT, cviVEncSetPred },
    { CVI_H26X_OP_SET_H264_ENTROPY, cviVEncSetH264Entropy },
    { CVI_H26X_OP_SET_H264_TRANS, cviVEncSetH264Trans },
    { CVI_H26X_OP_SET_H265_TRANS, cviVEncSetH265Trans },
    { CVI_H26X_OP_REG_VB_BUFFER, cviVEncRegReconBuf },
    { CVI_H26X_OP_SET_IN_PIXEL_FORMAT, cviVEncSetInPixelFormat },
    { CVI_H26X_OP_GET_CHN_INFO, cviVEncShowChnInfo },
    { CVI_H26X_OP_SET_USER_RC_INFO, cviVEncSetUserRcInfo },
    { CVI_H26X_OP_SET_SUPER_FRAME, cviVEncSetSuperFrame },
    { CVI_H26X_OP_SET_H264_VUI, cviVEncSetH264Vui },
    { CVI_H26X_OP_SET_H265_VUI, cviVEncSetH265Vui },
    { CVI_H26X_OP_SET_FRAME_PARAM, cviVEncSetFrameParam },
    { CVI_H26X_OP_CALC_FRAME_PARAM, NULL },
    { CVI_H26X_OP_SET_SB_MODE, NULL },
    { CVI_H26X_OP_START_SB_MODE, NULL },
    { CVI_H26X_OP_UPDATE_SB_WPTR, NULL },
    { CVI_H26X_OP_RESET_SB, NULL },
    { CVI_H26X_OP_SB_EN_DUMMY_PUSH, NULL },
    { CVI_H26X_OP_SB_TRIG_DUMMY_PUSH, NULL },
    { CVI_H26X_OP_SB_DIS_DUMMY_PUSH, NULL },
    { CVI_H26X_OP_SB_GET_SKIP_FRM_STATUS, NULL },
    { CVI_H26X_OP_SET_SBM_ENABLE, NULL },
    { CVI_H26X_OP_WAIT_FRAME_DONE, cviVEncWaitEncodeDone },
    { CVI_H26X_OP_SET_H264_SPLIT, cviVEncSetH264Split },
    { CVI_H26X_OP_SET_H265_SPLIT, cviVEncSetH265Split },
    { CVI_H26X_OP_SET_H264_DBLK, cviVEncSetH264DBlk },
    { CVI_H26X_OP_SET_H265_DBLK, cviVEncSetH265Dblk },
    { CVI_H26X_OP_SET_H264_INTRA_PRED, cviVEncSetH264IntraPred },
    { CVI_H26X_OP_SET_CUSTOM_MAP, cviVEncSetCustomMap },
    { CVI_H26X_OP_GET_INITIAL_INFO, cviVEncGetInitialInfo },
    { CVI_H26X_OP_SET_H265_SAO, cviVEncSetH265Sao},
    { CVI_H26X_OP_GET_ENCODE_HEADER, cviVEncGetEncodeHeader},
    { CVI_H26X_OP_SET_BIND_MODE, cviVEncSetBindMode},
    { CVI_H26X_OP_SET_H265_PRED_UNIT, cviVEncSetH265PredUnit},
    { CVI_H26X_OP_GET_H265_PRED_UNIT, cviVEncGetH265PredUnit},
    { CVI_H26X_OP_SET_SEARCH_WINDOW, cviVEncSetSearchWindow},
    { CVI_H26X_OP_GET_SEARCH_WINDOW, cviVEncGetSearchWindow},
};

int cviVEncIoctl(void *handle, int op, void *arg)
{
    int ret = 0;
    int currOp;

    if (op <= 0 || op >= CVI_H26X_OP_MAX) {
        VLOG(ERR, "op = %d\n", op);
        return -1;
    }

    currOp = (cviIoctlOp[op].opNum & CVI_H26X_OP_MASK) >> CVI_H26X_OP_SHIFT;
    if (op != currOp) {
        VLOG(ERR, "op = %d\n", op);
        return -1;
    }

    if (cviIoctlOp[op].ioctlFunc)
        ret = cviIoctlOp[op].ioctlFunc(handle, arg);

    return ret;
}

