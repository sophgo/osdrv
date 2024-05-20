/*
 * Copyright Cvitek Technologies Inc.
 *
 * Created Time: July, 2020
 */
#ifdef ENABLE_DEC
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <uapi/linux/sched/types.h>

#include "linux/cvi_comm_vdec.h"
#include "cvi_h265_interface.h"
#include "main_helper.h"
#include "vpuapi.h"
#include "vpuapifunc.h"
#include "vdi.h"
#include "debug.h"
#include "vincent.h"
#include "base_ctx.h"
#include "vb.h"
#include "linux/cvi_comm_video.h"
#include "vbq.h"
#include "bind.h"

#define IS_VALID_PA(pa) (((pa) != 0) && ((pa) != (PhysicalAddress)(-1)))
#define EXTRA_FRAME_BUFFER_NUM 1
#define MAX_DPB_NUM 31
#define USERDATA_SIZE 1024
#define MAX_WIDTH_AXI 4608
extern wait_queue_head_t tVdecWaitQueue[];

typedef struct _src_info {
    PhysicalAddress stream_addr;
    unsigned int stream_len;
}SRC_INFO;

typedef struct _timestamp_info {
    Uint64 pts;
    Uint64 dts;
}TIMESTAMP_INFO;

typedef struct _frame_info {
    unsigned char frame_idx;
    unsigned char interlacedFrame;
    unsigned char picType;
    unsigned int seqenceNo;
    unsigned int decHwTime;
}FRAME_INFO;

typedef enum {
    DISPLAY_MODE_PREVIEW,
    DISPLAY_MODE_PLAYBACK,
}DISPLAY_MODE;

typedef struct decoder_handle{
    DecHandle handle;
    DecOpenParam *open_param;
    FrameBuffer *pst_frame_buffer;
    VB_BLK *pst_frame_blk;
    DecInitialInfo *seq_info;
    DecGetFramebufInfo *fb_info;
    DecOutputInfo *output_info;
    int core_idx;
    int channel_index;
    SEQ_STATUS seq_status;
    int numOfDecFbc;
    int numOfDecwtl;
    TiledMapType map_type;
    Queue *display_frame;
    Queue *busy_src_buffer;
    Queue *free_src_buffer;
    int stop_wait_interrupt;
    void *thread_handle;
    PhysicalAddress bitstream_buffer[COMMAND_QUEUE_DEPTH];
    unsigned int bitstream_size;
    unsigned char bitstream_idx;
    unsigned char enable_userdata;
    PhysicalAddress userdata_buffer[COMMAND_QUEUE_DEPTH];
    unsigned int userdata_size;
    int frame_buffer_count;
    VB_INFO vb_info;
    FrameBufferFormat wtl_format;
    int cbcr_interleave;
    int nv21;
    Queue *timestamp_info;
    int is_bind_mode;
    struct vb_jobs_t jobs;
    int cmd_queue_depth;
    int stride_align;
    int user_pic_enable;
    int user_pic_mode;
    cviDispFrameCfg usr_pic;
    DISPLAY_MODE display_mode;
}DECODER_HANDLE;

int add_user_cnt(VB_BLK blk)
{
    struct vb_s *vb;

    if (blk == VB_INVALID_HANDLE)
        return -1;

    if (blk == 0)
        return -1;

    vb = (struct vb_s *)blk;
    atomic_fetch_add(1, &vb->usr_cnt);

    return 0;
}

int is_available(VB_BLK blk)
{
    struct vb_s *vb;

    if (blk == VB_INVALID_HANDLE)
        return 0;

    if (blk == 0)
        return 0;

    vb = (struct vb_s *)blk;
    if (atomic_read(&vb->usr_cnt) == 1)
        return 1;

    return 0;
}

int frame_bufer_add_user(void *pHandle, int frame_idx)
{
    VB_BLK blk;
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;

    if ((frame_idx < 0) || (frame_idx >= MAX_GDI_IDX))
            return 0;

    if (pst_handle->open_param->wtlEnable) {
        blk = pst_handle->pst_frame_blk[frame_idx + pst_handle->numOfDecFbc];
    } else {
        blk = pst_handle->pst_frame_blk[frame_idx];
    }

    return add_user_cnt(blk);
}

static void set_default_dec_param(DecOpenParam* pop)
{
    osal_memset(pop, 0, sizeof(DecOpenParam));

    pop->bitstreamMode   = BS_MODE_PIC_END;
    pop->streamEndian    = VPU_STREAM_ENDIAN;
    pop->frameEndian     = VPU_FRAME_ENDIAN;
    pop->cbcrInterleave  = FALSE;
    pop->nv21            = FALSE;
    pop->bitstreamFormat = STD_HEVC;
    pop->wtlEnable       = TRUE;
    pop->wtlMode         = FF_FRAME;
    pop->bitstreamBufferSize = 1024 * 1024;
}

static int alloc_framebuffer(void *pHandle)
{
    int ret;
    int frameBufferSize;
    int stride;
    int i;
    DRAMConfig DramCfg = {0};
    FrameBufferAllocInfo alloc_info = {0};
    PhysicalAddress addr;
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;
    FrameBufferFormat format;
    VB_BLK blk;

    pst_handle->numOfDecFbc = pst_handle->seq_info->minFrameBufferCount + pst_handle->cmd_queue_depth;
    pst_handle->numOfDecFbc += pst_handle->frame_buffer_count;
    if (pst_handle->numOfDecFbc > MAX_DPB_NUM) {
        pst_handle->numOfDecFbc = MAX_DPB_NUM;
        VLOG(ERR, "The number of decoding buffers exceeds the IP limit(%d,%d)\n", pst_handle->numOfDecFbc, MAX_DPB_NUM);
    }

    if (pst_handle->open_param->wtlEnable)
        pst_handle->numOfDecwtl = pst_handle->numOfDecFbc;
    else
        pst_handle->numOfDecwtl = 0;

    format = (pst_handle->seq_info->lumaBitdepth > 8 ||
              pst_handle->seq_info->chromaBitdepth > 8) ?
              FORMAT_420_P10_16BIT_LSB : FORMAT_420;
    stride = VPU_GetFrameBufStride(pst_handle->handle, pst_handle->seq_info->picWidth,
             pst_handle->seq_info->picHeight, format, 0, pst_handle->map_type);

    if (pst_handle->stride_align)
        stride = ALIGN(stride, pst_handle->stride_align);

    frameBufferSize = VPU_GetFrameBufSize(pst_handle->handle, pst_handle->core_idx, stride,
             pst_handle->seq_info->picHeight, pst_handle->map_type, format, 0, &DramCfg);

    for(i=0; i<pst_handle->numOfDecFbc; i++) {
        if (pst_handle->vb_info.vb_mode == VB_SOURCE_USER) {
            blk = vb_get_block_with_id(pst_handle->vb_info.frame_buffer_vb_pool, frameBufferSize, CVI_ID_VDEC);
            if (blk == VB_INVALID_HANDLE)
                blk = vb_get_block_with_id(VB_STATIC_POOLID, frameBufferSize, CVI_ID_VDEC);
        } else {
            blk = vb_get_block_with_id(VB_STATIC_POOLID, frameBufferSize, CVI_ID_VDEC);
        }

        if (blk == VB_INVALID_HANDLE)
            return RETCODE_FAILURE;

        addr = vb_handle2phys_addr(blk);
        if (addr == 0)
            return RETCODE_FAILURE;

        add_user_cnt(blk);
        pst_handle->pst_frame_buffer[i].bufY  = addr;
        pst_handle->pst_frame_buffer[i].bufCb = (PhysicalAddress)-1;
        pst_handle->pst_frame_buffer[i].bufCr = (PhysicalAddress)-1;
        pst_handle->pst_frame_buffer[i].updateFbInfo = TRUE;
        pst_handle->pst_frame_buffer[i].size  = frameBufferSize;
        pst_handle->pst_frame_buffer[i].width = pst_handle->seq_info->picWidth;
        pst_handle->pst_frame_buffer[i].height = pst_handle->seq_info->picHeight;
        pst_handle->pst_frame_buffer[i].stride = stride;
        pst_handle->pst_frame_blk[i] = blk;
    }

    alloc_info.nv21    = 0;
    alloc_info.format  = format;
    alloc_info.type    = FB_TYPE_CODEC;
    alloc_info.num     = pst_handle->numOfDecFbc;
    alloc_info.mapType = pst_handle->map_type;
    alloc_info.stride  = stride;
    alloc_info.height  = pst_handle->seq_info->picHeight;
    ret = VPU_DecAllocateFrameBuffer(pst_handle->handle, alloc_info, pst_handle->pst_frame_buffer);
    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "VPU_DecAllocateFrameBuffer failed! ret = %08x\n", ret);
        return ret;
    }

    if (pst_handle->open_param->wtlEnable) {
        stride = VPU_GetFrameBufStride(pst_handle->handle, pst_handle->seq_info->picWidth,
            pst_handle->seq_info->picHeight, FORMAT_420, 0, LINEAR_FRAME_MAP);

        if (pst_handle->stride_align)
            stride = ALIGN(stride, pst_handle->stride_align);

        frameBufferSize = VPU_GetFrameBufSize(pst_handle->handle, pst_handle->core_idx, stride,
            pst_handle->seq_info->picHeight, LINEAR_FRAME_MAP, FORMAT_420, 0, &DramCfg);

        for(i=pst_handle->numOfDecFbc; i<pst_handle->numOfDecFbc+pst_handle->numOfDecwtl; i++) {
            if (pst_handle->vb_info.vb_mode == VB_SOURCE_USER) {
                blk = vb_get_block_with_id(pst_handle->vb_info.frame_buffer_vb_pool, frameBufferSize, CVI_ID_VDEC);
                if (blk == VB_INVALID_HANDLE) {
                    blk = vb_get_block_with_id(VB_STATIC_POOLID, frameBufferSize, CVI_ID_VDEC);
                }
            } else {
                blk = vb_get_block_with_id(VB_STATIC_POOLID, frameBufferSize, CVI_ID_VDEC);
            }

            if (blk == VB_INVALID_HANDLE)
                return RETCODE_FAILURE;

            addr = vb_handle2phys_addr(blk);
            if (addr == 0)
                return RETCODE_FAILURE;

            add_user_cnt(blk);
            pst_handle->pst_frame_buffer[i].bufY  = addr;
            pst_handle->pst_frame_buffer[i].bufCb = (PhysicalAddress)-1;
            pst_handle->pst_frame_buffer[i].bufCr = (PhysicalAddress)-1;
            pst_handle->pst_frame_buffer[i].updateFbInfo = TRUE;
            pst_handle->pst_frame_buffer[i].size  = frameBufferSize;
            pst_handle->pst_frame_buffer[i].width = pst_handle->seq_info->picWidth;
            pst_handle->pst_frame_buffer[i].height = pst_handle->seq_info->picHeight;
            pst_handle->pst_frame_buffer[i].stride = stride;
            pst_handle->pst_frame_blk[i] = blk;
        }

        alloc_info.cbcrInterleave = pst_handle->cbcr_interleave;
        alloc_info.nv21    = pst_handle->nv21;
        alloc_info.format  = FORMAT_420;
        alloc_info.type    = FB_TYPE_CODEC;
        alloc_info.num     = pst_handle->numOfDecwtl;
        alloc_info.mapType = LINEAR_FRAME_MAP;
        alloc_info.stride  = stride;
        alloc_info.height  = pst_handle->seq_info->picHeight;
        ret = VPU_DecAllocateFrameBuffer(pst_handle->handle, alloc_info,
            &(pst_handle->pst_frame_buffer[pst_handle->numOfDecFbc]));
        if (ret != RETCODE_SUCCESS) {
            VLOG(ERR, "VPU_DecAllocateFrameBuffer failed! ret = %08x\n", ret);
            return ret;
        }
    }

    ret = VPU_DecRegisterFrameBufferEx(pst_handle->handle, pst_handle->pst_frame_buffer, pst_handle->numOfDecFbc,
        pst_handle->numOfDecwtl, stride, pst_handle->seq_info->picHeight, pst_handle->map_type);
    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "VPU_DecRegisterFrameBufferEx failed! ret = %08x\n", ret);
        return ret;
    }

    return 0;
}
static int free_framebuffer(void *pHandle)
{
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;
    int i;
    VB_BLK blk;
    struct vb_s *vb;
    int user_cnt;

    for (i=0; i<MAX_REG_FRAME; i++) {
        if (pst_handle->pst_frame_buffer[i].bufY == 0)
            continue;

        blk = pst_handle->pst_frame_blk[i];
        vb = (struct vb_s *)blk;
        do {
            user_cnt = atomic_read(&vb->usr_cnt);
            if (user_cnt > 1)
                atomic_sub(user_cnt-1, &vb->usr_cnt);
        } while(user_cnt != 1);

        vb_release_block(blk);
    }

    return 0;
}

static int release_framebuffer(void *pHandle)
{
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;
    int i = 0;
    int ret;
    int frame_idx;

    if (pst_handle->open_param->wtlEnable)
        i = pst_handle->numOfDecFbc;

    for (; i<MAX_REG_FRAME; i++) {
        if (pst_handle->pst_frame_blk[i] == 0)
            break;

        if (is_available(pst_handle->pst_frame_blk[i]))
            break;
    }

    if (pst_handle->pst_frame_blk[i] == 0)
        return 0;

    if (pst_handle->open_param->wtlEnable)
        frame_idx = i - pst_handle->numOfDecFbc;
    else
        frame_idx = i;

    ret = VPU_DecClrDispFlag(pst_handle->handle, frame_idx);
    if (ret == RETCODE_SUCCESS)
        add_user_cnt(pst_handle->pst_frame_blk[i]);

    return 0;
}

static int fill_vbbuffer(void *pHandle)
{
    VB_BLK blk;
    struct vb_s *vb;
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;
    FRAME_INFO *frame_info;
    int index_frame;
    struct cvi_buffer *vb_buf;
    MMF_CHN_S src_chn = {0};
    int ret;

    src_chn.enModId = CVI_ID_VDEC;
    src_chn.s32DevId = 0;
    src_chn.s32ChnId = pst_handle->channel_index;

    frame_info = (FRAME_INFO *)Queue_Peek(pst_handle->display_frame);
    if (frame_info == NULL) {
        return -1;
    }

    index_frame = frame_info->frame_idx;
    if (pst_handle->open_param->wtlEnable)
        index_frame += pst_handle->numOfDecFbc;

    blk = pst_handle->pst_frame_blk[index_frame];
    vb = (struct vb_s *)blk;
    vb_buf = &vb->buf;

    memset(vb_buf, 0, sizeof(struct cvi_buffer));
    vb_buf->size.u32Width = pst_handle->seq_info->picCropRect.right;
    vb_buf->size.u32Height = pst_handle->seq_info->picCropRect.bottom;
    vb_buf->stride[0] = pst_handle->pst_frame_buffer[index_frame].stride;
    if (pst_handle->pst_frame_buffer[index_frame].cbcrInterleave) {
        if (pst_handle->pst_frame_buffer[index_frame].nv21)
            vb_buf->enPixelFormat = PIXEL_FORMAT_NV21;
        else
            vb_buf->enPixelFormat = PIXEL_FORMAT_NV12;
        vb_buf->stride[1] = vb_buf->stride[0];
        vb_buf->stride[2] = 0;
    } else {
        vb_buf->enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;
        vb_buf->stride[1] = vb_buf->stride[0] >> 1;
        vb_buf->stride[2] = vb_buf->stride[0] >> 1;
    }

    if (pst_handle->open_param->wtlEnable) {//yuv
        vb_buf->phy_addr[0] = pst_handle->pst_frame_buffer[index_frame].bufY;
        vb_buf->phy_addr[1] = pst_handle->pst_frame_buffer[index_frame].bufCb;
        vb_buf->phy_addr[2] = pst_handle->pst_frame_buffer[index_frame].bufCr;

        vb_buf->length[0] = vb_buf->stride[0] * vb_buf->size.u32Height;
        vb_buf->length[1] = vb_buf->stride[1] * vb_buf->size.u32Height >> 1;
        vb_buf->length[2] = vb_buf->stride[2] * vb_buf->size.u32Height >> 1;
        vb_buf->enCompressMode = COMPRESS_MODE_NONE;
    } else {//fbc
        VPU_DecGiveCommand(pst_handle->handle, DEC_GET_FRAMEBUF_INFO, pst_handle->fb_info);
        vb_buf->phy_addr[0] = pst_handle->fb_info->vbFbcYTbl[index_frame].phys_addr;
        vb_buf->phy_addr[1] = pst_handle->fb_info->vbFbcCTbl[index_frame].phys_addr;
        vb_buf->phy_addr[2] = pst_handle->pst_frame_buffer[index_frame].bufY;
        vb_buf->compress_expand_addr = pst_handle->pst_frame_buffer[index_frame].bufCb;
        vb_buf->enCompressMode = COMPRESS_MODE_FRAME;
    }

    vb_buf->sequence = frame_info->seqenceNo;
    vb_buf->frame_crop.start_y = pst_handle->seq_info->picCropRect.top;
    vb_buf->frame_crop.end_y = pst_handle->seq_info->picCropRect.bottom;
    vb_buf->frame_crop.start_x = pst_handle->seq_info->picCropRect.left;
    vb_buf->frame_crop.end_x = pst_handle->seq_info->picCropRect.right;

    ret = vb_done_handler(src_chn, CHN_TYPE_OUT, &pst_handle->jobs, blk);
    if (ret == CVI_SUCCESS) {
        Queue_Dequeue(pst_handle->display_frame);
    } else if (pst_handle->display_mode == DISPLAY_MODE_PREVIEW) {
        vb_release_block(blk);
        Queue_Dequeue(pst_handle->display_frame);
    }

    return RETCODE_SUCCESS;
}

static int sequence_change(void *pHandle)
{
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;
    int ret;

    VPU_DecGiveCommand(pst_handle->handle, DEC_GET_FRAMEBUF_INFO, pst_handle->fb_info);
    VPU_DecGiveCommand(pst_handle->handle, DEC_FREE_FRAME_BUFFER, NULL);
    free_framebuffer(pst_handle);
    VPU_DecGiveCommand(pst_handle->handle, DEC_RESET_FRAMEBUF_INFO, NULL);
    memset(pst_handle->pst_frame_buffer, 0, MAX_REG_FRAME*sizeof(FrameBuffer));
    VPU_DecGiveCommand(pst_handle->handle, DEC_GET_SEQ_INFO, pst_handle->seq_info);
    ret = alloc_framebuffer(pst_handle);
    if (ret ==  RETCODE_SUCCESS)
        pst_handle->seq_status = SEQ_DECODE_START;
    else
        pst_handle->seq_status = SEQ_DECODE_FINISH;

    return ret;
}

static int thread_decode(void *param)
{
    int ret;
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)param;
    QueueStatusInfo queue_status = {0};
    SRC_INFO *src_info;
    unsigned char decode_one_frame = 0;
    int report_queue_cnt = 0;
    SecAxiUse  secAxiUse = {0};
    DecParam dec_param = {0};
    MMF_CHN_S src_chn = {0};
    MMF_BIND_DEST_S bind_dst = {0};
    int cyclePerTick = 256;

    src_chn.enModId = CVI_ID_VDEC;
    src_chn.s32DevId = 0;
    src_chn.s32ChnId = pst_handle->channel_index;

    while(1) {
        VPU_DecGiveCommand(pst_handle->handle, DEC_GET_QUEUE_STATUS, &queue_status);
        if (pst_handle->stop_wait_interrupt) {
            FRAME_INFO *frame_info;
            if (!queue_status.instanceQueueCount && queue_status.reportQueueEmpty)
                break;

            frame_info = (FRAME_INFO *)Queue_Dequeue(pst_handle->display_frame);
            if (frame_info != NULL) {
                VPU_DecClrDispFlag(pst_handle->handle, frame_info->frame_idx);
            }

            decode_one_frame = 0;
            VPU_DecFrameBufferFlush(pst_handle->handle, NULL, NULL);
        } else if (pst_handle->user_pic_enable) {
            if (pst_handle->user_pic_mode) {
                wake_up(&tVdecWaitQueue[pst_handle->channel_index]);
                msleep(10);
            } else if (pst_handle->seq_status == SEQ_DECODE_FINISH) {
                wake_up(&tVdecWaitQueue[pst_handle->channel_index]);
                msleep(10);
            }
        }

        if (pst_handle->is_bind_mode)
            fill_vbbuffer(pst_handle);

        if (pst_handle->seq_status > SEQ_INIT_START)
            release_framebuffer(pst_handle);

        if (pst_handle->seq_status == SEQ_CHANGE) {
            if (Queue_Get_Cnt(pst_handle->display_frame)) {
                msleep(10);
                continue;
            } else {
                if (RETCODE_SUCCESS != sequence_change(pst_handle))
                    break;
            }
        }

        if ((!decode_one_frame) &&
            (!queue_status.instanceQueueFull) &&
            (pst_handle->seq_status != SEQ_CHANGE) &&
            (pst_handle->seq_status != SEQ_DECODE_FINISH) &&
            (!pst_handle->stop_wait_interrupt)) {
            if (pst_handle->open_param->bitstreamMode == BS_MODE_PIC_END) {
                src_info = (SRC_INFO *)Queue_Dequeue(pst_handle->busy_src_buffer);
                if (src_info) {
                    VPU_DecSetRdPtr(pst_handle->handle, src_info->stream_addr, TRUE);
                    VPU_DecUpdateBitstreamBuffer(pst_handle->handle, src_info->stream_len);
                    decode_one_frame = 1;
                }
            } else if (pst_handle->open_param->bitstreamMode == BS_MODE_INTERRUPT) {
                if (pst_handle->seq_status != SEQ_DECODE_FINISH)
                    decode_one_frame = 1;
            }
        }

        if (decode_one_frame) {
            if (pst_handle->seq_status == SEQ_INIT_NON) {
                pst_handle->seq_status = SEQ_INIT_START;
                ret = VPU_DecIssueSeqInit(pst_handle->handle);
                if (RETCODE_SUCCESS != ret) {
                    VLOG(ERR, "VPU_DecIssueSeqInit failed! ret=%08x\n", ret);
                    return ret;
                }
            }

            if (pst_handle->seq_status == SEQ_DECODE_START) {
                ret = VPU_DecStartOneFrame(pst_handle->handle, &dec_param);
                if (ret == RETCODE_SUCCESS)
                    decode_one_frame = 0;
                continue;
            }
        }

        ret = VPU_WaitInterruptEx(pst_handle->handle, 1);
        if (ret < 0) {
            continue;
        }

        if (ret > 0) {
            VPU_ClearInterruptEx(pst_handle->handle, ret);
        }

        if (ret & (1<<INT_WAVE5_INIT_SEQ)){
            ret = VPU_DecCompleteSeqInit(pst_handle->handle, pst_handle->seq_info);
            if (ret != RETCODE_SUCCESS) {
                VLOG(ERR, "VPU_DecCompleteSeqInit failed! ret = %08x\n", ret);
                decode_one_frame = 0;
                pst_handle->seq_status = SEQ_INIT_NON;

                Queue_Enqueue(pst_handle->free_src_buffer, &pst_handle->bitstream_buffer[pst_handle->bitstream_idx]);
                pst_handle->bitstream_idx++;
                pst_handle->bitstream_idx %= pst_handle->cmd_queue_depth;

                continue;
            }

            if (pst_handle->open_param->wtlEnable == TRUE)
                VPU_DecGiveCommand(pst_handle->handle, DEC_SET_WTL_FRAME_FORMAT, (void *)&pst_handle->wtl_format);

            if (pst_handle->seq_info->picWidth <= MAX_WIDTH_AXI) {
                secAxiUse.u.wave.useIpEnable    = TRUE;
                secAxiUse.u.wave.useLfRowEnable = TRUE;
                secAxiUse.u.wave.useBitEnable   = TRUE;
            }
            VPU_DecGiveCommand(pst_handle->handle, SET_SEC_AXI, &secAxiUse);

            ret = alloc_framebuffer(pst_handle);
            if (ret != RETCODE_SUCCESS) {
                pst_handle->seq_status = SEQ_DECODE_FINISH;
                VLOG(ERR, "alloc_framebuffer failed! ret = %08x\n",ret);
                break;
            }

            pst_handle->seq_status = SEQ_DECODE_START;
            ret = bind_get_dst(&src_chn, &bind_dst);
            if (ret == 0)
                pst_handle->is_bind_mode = 1;
        } else if (ret & (1<<INT_WAVE5_DEC_PIC)) {
            report_queue_cnt = 0;
            do {
                ret = VPU_DecGetOutputInfo(pst_handle->handle, pst_handle->output_info);
                if (ret != RETCODE_SUCCESS) {
                    msleep(1);
                    report_queue_cnt++;
                }
            } while ((ret != RETCODE_SUCCESS) && (report_queue_cnt<3));

            if (ret != RETCODE_SUCCESS)
                continue;

            if (pst_handle->output_info->sequenceChanged)
                pst_handle->seq_status = SEQ_CHANGE;

            if (pst_handle->output_info->indexFrameDisplay == DISPLAY_IDX_FLAG_SEQ_END)
                pst_handle->seq_status = SEQ_DECODE_FINISH;

            if (pst_handle->output_info->indexFrameDisplay >= 0) {
                if (pst_handle->stop_wait_interrupt)
                    VPU_DecClrDispFlag(pst_handle->handle, pst_handle->output_info->indexFrameDisplay);
                else {
                    FRAME_INFO frame_info;
                    frame_info.frame_idx = pst_handle->output_info->indexFrameDisplay;
                    frame_info.seqenceNo = pst_handle->output_info->sequenceNo;
                    frame_info.picType = pst_handle->output_info->picType;
                    frame_info.interlacedFrame = pst_handle->output_info->interlacedFrame;
                    frame_info.decHwTime =
                        (pst_handle->output_info->decDecodeEndTick - pst_handle->output_info->decHostCmdTick)*cyclePerTick/(VPU_STAT_CYCLES_CLK/1000000);
                    Queue_Enqueue(pst_handle->display_frame, &frame_info);
                    wake_up(&tVdecWaitQueue[pst_handle->channel_index]);
                }
            }

            if (pst_handle->open_param->bitstreamMode == BS_MODE_PIC_END) {
                if ((pst_handle->output_info->indexFrameDecoded >= 0) ||
                    (pst_handle->output_info->indexFrameDecoded == DECODED_IDX_FLAG_SKIP)) {
                    Queue_Enqueue(pst_handle->free_src_buffer, &pst_handle->bitstream_buffer[pst_handle->bitstream_idx]);
                    pst_handle->bitstream_idx++;
                    pst_handle->bitstream_idx %= pst_handle->cmd_queue_depth;
                }
            }
        }
    }

    VPU_DecGiveCommand(pst_handle->handle, DEC_FREE_FRAME_BUFFER, NULL);
    free_framebuffer(pst_handle);
    VPU_DecGiveCommand(pst_handle->handle, DEC_RESET_FRAMEBUF_INFO, NULL);
    pst_handle->thread_handle  = NULL;

    return 0;
}

int cviVDecOpen(cviInitDecConfig *pInitDecCfg, void **pHandle)
{
    int                 fw_size;
    Uint16*             pus_bitCode;
    int                 core_idx = 1;
    int                    ret;
    vpu_buffer_t        vb_buffer;
    int                    cyclePerTick = 32768;
    DECODER_HANDLE        *pst_handle;
    VpuAttr                product_attr;
    int                    i;

    core_idx = VPU_DecGetCoreIdx();

    pus_bitCode = (Uint16*)bit_code;
    fw_size = sizeof(bit_code) / sizeof(bit_code[0]);

    ret = VPU_InitWithBitcode(core_idx, pus_bitCode, fw_size);
    if ((ret != RETCODE_SUCCESS) && (ret != RETCODE_CALLED_BEFORE)) {
        VLOG(ERR, "VPU_InitWithBitcode failed! ret = %08x\n", ret);
        return ret;
    }

    ret = PrintVpuProductInfo(core_idx, &product_attr);
    if (ret == RETCODE_VPU_RESPONSE_TIMEOUT ) {
        VLOG(ERR, "PrintVpuProductInfo failed! ret = %08x\n", ret);
        return ret;
    }

    pst_handle = osal_calloc(1, sizeof(DECODER_HANDLE));
    if (pst_handle == NULL) {
        goto fail;
    }

    pst_handle->pst_frame_buffer = osal_calloc(1, MAX_REG_FRAME*sizeof(FrameBuffer));
    if (pst_handle->pst_frame_buffer == NULL) {
        goto fail;
    }

    pst_handle->pst_frame_blk = osal_calloc(1, MAX_REG_FRAME*sizeof(VB_BLK));
    if (pst_handle->pst_frame_blk == NULL) {
        goto fail;
    }

    pst_handle->fb_info = osal_calloc(1, sizeof(DecGetFramebufInfo));
        if (pst_handle->fb_info == NULL) {
        goto fail;
    }

    pst_handle->seq_info = osal_calloc(1, sizeof(DecInitialInfo));
        if (pst_handle->seq_info == NULL) {
        goto fail;
    }

    pst_handle->output_info = osal_calloc(1, sizeof(DecOutputInfo));
        if (pst_handle->output_info == NULL) {
        goto fail;
    }

    pst_handle->open_param = osal_calloc(1, sizeof(DecOpenParam));
        if (pst_handle->open_param == NULL) {
        goto fail;
    }

    if ((pInitDecCfg->cmdQueueDepth > 0) && (pInitDecCfg->cmdQueueDepth <= COMMAND_QUEUE_DEPTH))
        pst_handle->cmd_queue_depth = pInitDecCfg->cmdQueueDepth;
    else
        pst_handle->cmd_queue_depth = COMMAND_QUEUE_DEPTH;
    set_default_dec_param(pst_handle->open_param);
    pst_handle->timestamp_info = Queue_Create_With_Lock(32, sizeof(TIMESTAMP_INFO));
    pst_handle->display_frame = Queue_Create_With_Lock(32, sizeof(FRAME_INFO));
    pst_handle->busy_src_buffer = Queue_Create_With_Lock(pst_handle->cmd_queue_depth, sizeof(SRC_INFO));
    pst_handle->free_src_buffer = Queue_Create_With_Lock(pst_handle->cmd_queue_depth, sizeof(PhysicalAddress));
    pst_handle->map_type = COMPRESSED_FRAME_MAP;
    pst_handle->core_idx = core_idx;
    pst_handle->frame_buffer_count = pInitDecCfg->frameBufferCount;
    pst_handle->open_param->coreIdx = core_idx;
    pst_handle->open_param->bitstreamMode = pInitDecCfg->BsMode;
    pst_handle->channel_index = pInitDecCfg->chnNum;
    pst_handle->wtl_format = FORMAT_420;
    pst_handle->open_param->wtlEnable = pInitDecCfg->wtl_enable;
    if (pInitDecCfg->codec == CODEC_H265)
        pst_handle->open_param->bitstreamFormat = STD_HEVC;
    else
        pst_handle->open_param->bitstreamFormat = STD_AVC;

    if (pInitDecCfg->bsBufferSize)
        vb_buffer.size = pInitDecCfg->bsBufferSize;
    else
        vb_buffer.size = pst_handle->open_param->bitstreamBufferSize;

    if (pst_handle->open_param->bitstreamMode == BS_MODE_PIC_END) {
        for (i=0; i<pst_handle->cmd_queue_depth; i++) {
            vdi_allocate_dma_memory(core_idx, &vb_buffer, 0, 0);
            pst_handle->bitstream_buffer[i] = vb_buffer.phys_addr;
            Queue_Enqueue(pst_handle->free_src_buffer, &vb_buffer.phys_addr);
        }
        pst_handle->bitstream_size = vb_buffer.size;
        pst_handle->open_param->bitstreamBufferSize = 0;
        pst_handle->open_param->bitstreamBuffer = 0;

    } else if (pst_handle->open_param->bitstreamMode == BS_MODE_INTERRUPT) {
        vdi_allocate_dma_memory(core_idx, &vb_buffer, 0, 0);
        pst_handle->bitstream_buffer[0] = vb_buffer.phys_addr;
        pst_handle->bitstream_size = vb_buffer.size;
        pst_handle->open_param->bitstreamBufferSize = vb_buffer.size;
        pst_handle->open_param->bitstreamBuffer = pst_handle->bitstream_buffer[0];
    }
    pst_handle->open_param->cmdQueueDepth = pst_handle->cmd_queue_depth;

    ret = VPU_DecOpen(&pst_handle->handle, pst_handle->open_param);
    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "Failed to VPU_DecOpen(ret:%d)\n", ret);
        goto fail;
    }

    if (pst_handle->enable_userdata) {
        pst_handle->userdata_size = (USERDATA_SIZE);
        osal_memset(&vb_buffer, 0, sizeof(vpu_buffer_t));
        vb_buffer.size = pst_handle->userdata_size;
        for (i=0; i<pst_handle->cmd_queue_depth; i++) {
            vdi_allocate_dma_memory(core_idx, &vb_buffer, 0, 0);
            pst_handle->userdata_buffer[i] = vb_buffer.phys_addr;
        }
    }

    if (TRUE == product_attr.supportNewTimer)
        cyclePerTick = 256;
    VPU_DecGiveCommand(pst_handle->handle, SET_CYCLE_PER_TICK,     (void *)&cyclePerTick);
    base_mod_jobs_init(&pst_handle->jobs, 1, 1, 0);

    *pHandle = pst_handle;

    return RETCODE_SUCCESS;

fail:
    for (i=0; i<pst_handle->cmd_queue_depth; i++) {
        vb_buffer.size = pst_handle->open_param->bitstreamBufferSize;
        vb_buffer.phys_addr = pst_handle->bitstream_buffer[i];
        vdi_free_dma_memory(pst_handle->core_idx, &vb_buffer, 0, 0);
    }

    for (i=0; i<pst_handle->cmd_queue_depth; i++) {
        vb_buffer.size = pst_handle->userdata_size;
        if (pst_handle->enable_userdata) {
            vb_buffer.phys_addr = pst_handle->userdata_buffer[i];
            vdi_free_dma_memory(pst_handle->core_idx, &vb_buffer, 0, 0);
        }
    }

    Queue_Destroy(pst_handle->display_frame);
    Queue_Destroy(pst_handle->busy_src_buffer);
    Queue_Destroy(pst_handle->free_src_buffer);
    Queue_Destroy(pst_handle->timestamp_info);

    if (pst_handle->pst_frame_buffer)
        vfree(pst_handle->pst_frame_buffer);

    if (pst_handle->pst_frame_blk)
        vfree(pst_handle->pst_frame_blk);

    if (pst_handle->fb_info)
        vfree(pst_handle->fb_info);

    if (pst_handle->seq_info)
        vfree(pst_handle->seq_info);

    if (pst_handle->output_info)
        vfree(pst_handle->output_info);

    if (pst_handle->open_param)
        vfree(pst_handle->open_param);

    if (pst_handle)
        vfree(pst_handle);

    return ret;
}

int cviVDecClose(void *pHandle)
{
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;
    vpu_buffer_t vb;
    int i;
    int ret;

    if (pst_handle->thread_handle != NULL) {
        pst_handle->stop_wait_interrupt = 1;
        osal_thread_join(pst_handle->thread_handle, NULL);
        pst_handle->thread_handle  = NULL;
    }

    for (i=0; i<pst_handle->cmd_queue_depth; i++) {
        vb.size = pst_handle->bitstream_size;
        vb.phys_addr = pst_handle->bitstream_buffer[i];
        if (vb.phys_addr)
            vdi_free_dma_memory(pst_handle->core_idx, &vb, 0, 0);
    }

    for (i=0; i<pst_handle->cmd_queue_depth; i++) {
        vb.size = pst_handle->userdata_size;
        if (pst_handle->enable_userdata) {
            vb.phys_addr = pst_handle->userdata_buffer[i];
            vdi_free_dma_memory(pst_handle->core_idx, &vb, 0, 0);
        }
    }

    do {
        VPU_DecFrameBufferFlush(pst_handle->handle, NULL, NULL);
        ret = VPU_DecClose(pst_handle->handle);
    } while (ret == RETCODE_VPU_STILL_RUNNING);

    VPU_DeInit(pst_handle->core_idx);

    Queue_Destroy(pst_handle->display_frame);
    Queue_Destroy(pst_handle->busy_src_buffer);
    Queue_Destroy(pst_handle->free_src_buffer);
    Queue_Destroy(pst_handle->timestamp_info);

    if (pst_handle->pst_frame_buffer)
        vfree(pst_handle->pst_frame_buffer);

    if (pst_handle->pst_frame_blk)
        vfree(pst_handle->pst_frame_blk);

    if (pst_handle->fb_info)
        vfree(pst_handle->fb_info);

    if (pst_handle->seq_info)
        vfree(pst_handle->seq_info);

    if (pst_handle->output_info)
        vfree(pst_handle->output_info);

    if (pst_handle->open_param)
        vfree(pst_handle->open_param);

    base_mod_jobs_exit(&pst_handle->jobs);
    osal_free(pst_handle);
    return 0;
}

int cviVDecReset(void *pHandle)
{

    return 0;
}

int cviVDecDecOnePic(void *pHandle, cviDecOnePicCfg *pdopc, int timeout_ms)
{
    PhysicalAddress rd_ptr;
    PhysicalAddress wr_ptr;
    Uint32 room = 0;
    Uint32 temp_room = 0;
    int ret = RETCODE_SUCCESS;
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;
    PhysicalAddress *buffer_addr;
    QueueStatusInfo queue_status = {0};
    SRC_INFO src_info;
    TIMESTAMP_INFO timestamp_info;

    if (pst_handle->user_pic_enable && pdopc->bsLen)
        return RETCODE_QUEUEING_FAILURE;

    if (pst_handle->enable_userdata) {
        *buffer_addr = pst_handle->userdata_buffer[pst_handle->bitstream_idx];
        VPU_DecGiveCommand(pst_handle->handle, SET_ADDR_REP_USERDATA, (void *)&buffer_addr);
        VPU_DecGiveCommand(pst_handle->handle, SET_SIZE_REP_USERDATA, (void *)&pst_handle->userdata_size);
        VPU_DecGiveCommand(pst_handle->handle, ENABLE_REP_USERDATA,   (void *)&pst_handle->enable_userdata);
    }

    if (pst_handle->open_param->bitstreamMode == BS_MODE_PIC_END) {
        VPU_DecGiveCommand(pst_handle->handle, DEC_GET_QUEUE_STATUS, &queue_status);
        if (queue_status.instanceQueueFull)
            return RETCODE_QUEUEING_FAILURE;

        if (pdopc->bsLen == 0) {
            if (queue_status.instanceQueueCount || Queue_Get_Cnt(pst_handle->display_frame))
                return RETCODE_QUEUEING_FAILURE;
            else if (pst_handle->seq_status == SEQ_DECODE_FINISH)
                return RETCODE_SUCCESS;
        }

        buffer_addr = (PhysicalAddress *)Queue_Dequeue(pst_handle->free_src_buffer);
        if (buffer_addr == NULL)
            return RETCODE_QUEUEING_FAILURE;

        if (pst_handle->bitstream_size < pdopc->bsLen) {
            VLOG(ERR, "stream buffer full\n");
            return -1;
        }

        vdi_write_memory(pst_handle->core_idx, *buffer_addr, pdopc->bsAddr, pdopc->bsLen, HOST_ENDIAN);
        src_info.stream_addr = *buffer_addr;
        src_info.stream_len = pdopc->bsLen;
        Queue_Enqueue(pst_handle->busy_src_buffer, &src_info);
        if (pdopc->bsLen) {
            timestamp_info.pts = pdopc->pts;
            timestamp_info.dts = pdopc->dts;
            Queue_Enqueue(pst_handle->timestamp_info, (void *)&timestamp_info);
        }

        if (pdopc->bsLen == 0) {
            if (pst_handle->seq_status != SEQ_DECODE_FINISH)
                return RETCODE_QUEUEING_FAILURE;
        }
    } else if (pst_handle->open_param->bitstreamMode == BS_MODE_INTERRUPT) {
        VPU_DecGetBitstreamBuffer(pst_handle->handle, &rd_ptr, &wr_ptr, &room);
        if (room < pdopc->bsLen)
            return RETCODE_QUEUEING_FAILURE;

        if (pdopc->bsLen > 0) {
            if (wr_ptr + pdopc->bsLen > pst_handle->bitstream_buffer[0] + pst_handle->bitstream_size) {
                temp_room = pst_handle->bitstream_buffer[0] + pst_handle->bitstream_size - wr_ptr;
                vdi_write_memory(pst_handle->core_idx, wr_ptr, pdopc->bsAddr, temp_room, HOST_ENDIAN);
                wr_ptr = pst_handle->bitstream_buffer[0];
                vdi_write_memory(pst_handle->core_idx, wr_ptr, pdopc->bsAddr + temp_room,
                    pdopc->bsLen - temp_room, HOST_ENDIAN);
            } else {
                vdi_write_memory(pst_handle->core_idx, wr_ptr, pdopc->bsAddr, pdopc->bsLen, HOST_ENDIAN);
            }
        }

        ret = VPU_DecUpdateBitstreamBuffer(pst_handle->handle, pdopc->bsLen);
        if (ret != RETCODE_SUCCESS) {
            VLOG(ERR, "VPU_DecUpdateBitstreamBuffer failed! ret = %08x\n",ret);
            return ret;
        }

        if (pdopc->bsLen == 0) {
            if ((pst_handle->seq_status != SEQ_DECODE_FINISH) || Queue_Get_Cnt(pst_handle->display_frame))
                return RETCODE_QUEUEING_FAILURE;
        }
    }

    if (pst_handle->thread_handle == NULL) {
        struct sched_param param = {
            .sched_priority = 95,
        };
        // pst_handle->thread_handle = osal_thread_create(thread_decode, (void *)pst_handle);
        pst_handle->thread_handle = kthread_run(thread_decode, pst_handle, "vdec_core%d_%d", pst_handle->core_idx, pst_handle->channel_index);
        if (IS_ERR(pst_handle->thread_handle))
            pst_handle->thread_handle = NULL;
        else
            sched_setscheduler(pst_handle->thread_handle, SCHED_RR, &param);
    }

    return ret;
}

int get_user_pic(void *pHandle, cviDispFrameCfg *pdfc)
{
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;

    memcpy(pdfc, &pst_handle->usr_pic, sizeof(cviDispFrameCfg));

    return 0;
}

int get_codec_pic(void *pHandle, cviDispFrameCfg *pdfc)
{
    FRAME_INFO *frame_info;
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;
    TIMESTAMP_INFO *timestamp_info;
    int index_frame;

    frame_info = (FRAME_INFO *)Queue_Dequeue(pst_handle->display_frame);
    if (frame_info == NULL) {
        return -1;
    }

    pdfc->width = pst_handle->seq_info->picCropRect.right;
    pdfc->height = pst_handle->seq_info->picCropRect.bottom;
    index_frame = pdfc->indexFrameDisplay = frame_info->frame_idx;
    if (pst_handle->open_param->wtlEnable) {//yuv
        index_frame += pst_handle->numOfDecFbc;
        pdfc->cbcrInterleave = pst_handle->pst_frame_buffer[index_frame].cbcrInterleave;
        pdfc->nv21 = pst_handle->pst_frame_buffer[index_frame].nv21;
        pdfc->strideY = pst_handle->pst_frame_buffer[index_frame].stride;
        if (pdfc->cbcrInterleave)
            pdfc->strideC = pdfc->strideY;
        else
            pdfc->strideC = pdfc->strideY >> 1;
        pdfc->phyAddr[0] = pst_handle->pst_frame_buffer[index_frame].bufY;
        pdfc->phyAddr[1] = pst_handle->pst_frame_buffer[index_frame].bufCb;
        pdfc->phyAddr[2] = pst_handle->pst_frame_buffer[index_frame].bufCr;
        pdfc->addr[0] = phys_to_virt(pdfc->phyAddr[0]);
        pdfc->addr[1] = phys_to_virt(pdfc->phyAddr[1]);
        pdfc->addr[2] = phys_to_virt(pdfc->phyAddr[2]);
        pdfc->length[0] = pdfc->strideY * pdfc->height;
        pdfc->length[1] = pdfc->strideC * pdfc->height >> 1;
        pdfc->length[2] = (!pdfc->cbcrInterleave) ? pdfc->length[1] : 0;
        pdfc->bCompressFrame = 0;;
    } else {//fbc
        VPU_DecGiveCommand(pst_handle->handle, DEC_GET_FRAMEBUF_INFO, pst_handle->fb_info);
        pdfc->cbcrInterleave = pst_handle->pst_frame_buffer[index_frame].cbcrInterleave;
        pdfc->nv21 = pst_handle->pst_frame_buffer[index_frame].nv21;
        pdfc->strideY = pst_handle->pst_frame_buffer[index_frame].stride;
        if (pdfc->cbcrInterleave)
            pdfc->strideC = pdfc->strideY;
        else
            pdfc->strideC = pdfc->strideY >> 1;
        pdfc->phyAddr[0] = pst_handle->pst_frame_buffer[index_frame].bufY;
        pdfc->phyAddr[1] = pst_handle->pst_frame_buffer[index_frame].bufCb;
        pdfc->phyAddr[2] = pst_handle->fb_info->vbFbcYTbl[index_frame].phys_addr;
        pdfc->phyAddr[3] = pst_handle->fb_info->vbFbcCTbl[index_frame].phys_addr;
        pdfc->addr[0] = phys_to_virt(pdfc->phyAddr[0]);
        pdfc->addr[1] = phys_to_virt(pdfc->phyAddr[1]);
        pdfc->addr[2] = phys_to_virt(pdfc->phyAddr[2]);
        pdfc->addr[3] = phys_to_virt(pdfc->phyAddr[3]);
        pdfc->length[0] = pdfc->strideY * pdfc->height;
        pdfc->length[1] = VPU_ALIGN16(pdfc->strideC)*pdfc->height;
        pdfc->length[2] = CalculateAuxBufferSize(AUX_BUF_TYPE_FBC_Y_OFFSET,
            pst_handle->open_param->bitstreamFormat, pdfc->width, pdfc->height);
        pdfc->length[3] = CalculateAuxBufferSize(AUX_BUF_TYPE_FBC_C_OFFSET,
            pst_handle->open_param->bitstreamFormat, pdfc->width, pdfc->height);
        pdfc->bCompressFrame = 1;
    }

    if (pst_handle->open_param->bitstreamMode == BS_MODE_PIC_END) {
        timestamp_info = (TIMESTAMP_INFO *)Queue_Dequeue(pst_handle->timestamp_info);
        if (timestamp_info) {
            pdfc->pts = timestamp_info->pts;
            pdfc->dts = timestamp_info->dts;
        }
    }

    pdfc->endian = HOST_ENDIAN;
    pdfc->picType = frame_info->picType;
    pdfc->seqenceNo = frame_info->seqenceNo;
    pdfc->interlacedFrame = frame_info->interlacedFrame;
    pdfc->decHwTime = frame_info->decHwTime;

    return 0;
}

int cviVDecGetFrame(void *pHandle, cviDispFrameCfg *pdfc)
{
    FRAME_INFO *frame_info;
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;
    TIMESTAMP_INFO *timestamp_info;
    int index_frame;

    if (!pst_handle->user_pic_enable) //disable user pic
        return get_codec_pic(pHandle, pdfc);

    if (!pst_handle->user_pic_mode &&
        pst_handle->seq_status != SEQ_DECODE_FINISH &&
        Queue_Get_Cnt(pst_handle->display_frame)) //enable user pic and instant=0
        return get_codec_pic(pHandle, pdfc);

    return get_user_pic(pHandle, pdfc);
}

void cviVDecReleaseFrame(void *pHandle, void *arg, PhysicalAddress addr)
{
    int frame_idx = (uintptr_t)arg;
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;
    PhysicalAddress _addr;
    VB_BLK blk;
    struct vb_s *vb;

    if ((frame_idx < 0) || (frame_idx >= MAX_GDI_IDX))
        return ;

    if (pst_handle->open_param->wtlEnable) {
        _addr = pst_handle->pst_frame_buffer[frame_idx + pst_handle->numOfDecFbc].bufY;
        blk = pst_handle->pst_frame_blk[frame_idx + pst_handle->numOfDecFbc];
    } else {
        _addr = pst_handle->pst_frame_buffer[frame_idx].bufY;
        blk = pst_handle->pst_frame_blk[frame_idx];
    }

    if (_addr != addr) {
        return ;
    }

    vb = (struct vb_s *)blk;

    if (atomic_read(&vb->usr_cnt) == 1)
        return ;

    vb_release_block(blk);

    return ;
}

void cviVDecAttachFrmBuf(void *pHandle, VB_INFO vb_info)
{
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;

    pst_handle->vb_info = vb_info;

    return;
}

int cviVdecGetFrameDisplayCount(void *pHandle)
{
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;

    if (pst_handle->user_pic_enable) {
        if (pst_handle->user_pic_mode)
            return 1;

        if (pst_handle->seq_status == SEQ_DECODE_FINISH)
            return 1;
    }

    return Queue_Get_Cnt(pst_handle->display_frame);
}

void cviVDecAttachCallBack(CVI_VDEC_DRV_CALLBACK pCbFunc)
{
    return;
}

void set_cbcr_format(void *pHandle, int cbcrInterleave, int nv21)
{
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;

    pst_handle->cbcr_interleave = cbcrInterleave;
    pst_handle->nv21 = nv21;
    VPU_DecSetCbCr(pst_handle->handle, cbcrInterleave, nv21);

    return ;
}

void get_status(void *pHandle, void* status)
{
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;
    VDEC_CHN_STATUS_S *_status = status;

    _status->u8BusySrcBuffer = Queue_Get_Cnt(pst_handle->busy_src_buffer);
    _status->u8FreeSrcBuffer = Queue_Get_Cnt(pst_handle->free_src_buffer);
    _status->u8Status = pst_handle->seq_status;

    _status->stSeqinitalInfo.s32PicWidth = pst_handle->seq_info->picWidth;
    _status->stSeqinitalInfo.s32PicHeight = pst_handle->seq_info->picHeight;
    _status->stSeqinitalInfo.s32FRateNumerator = pst_handle->seq_info->fRateNumerator;
    _status->stSeqinitalInfo.s32FRateDenominator = pst_handle->seq_info->fRateDenominator;
    _status->stSeqinitalInfo.stPicCropRect.top = pst_handle->seq_info->picCropRect.top;
    _status->stSeqinitalInfo.stPicCropRect.bottom = pst_handle->seq_info->picCropRect.bottom;
    _status->stSeqinitalInfo.stPicCropRect.right = pst_handle->seq_info->picCropRect.right;
    _status->stSeqinitalInfo.stPicCropRect.left = pst_handle->seq_info->picCropRect.left;
    _status->stSeqinitalInfo.s32MinFrameBufferCount = pst_handle->seq_info->minFrameBufferCount;
    _status->stSeqinitalInfo.s32FrameBufDelay = pst_handle->seq_info->frameBufDelay;
    _status->stSeqinitalInfo.s32Profile = pst_handle->seq_info->profile;
    _status->stSeqinitalInfo.s32Level = pst_handle->seq_info->level;
    _status->stSeqinitalInfo.s32Interlace = pst_handle->seq_info->interlace;
    _status->stSeqinitalInfo.s32MaxNumRefFrmFlag = pst_handle->seq_info->maxNumRefFrmFlag;
    _status->stSeqinitalInfo.s32MaxNumRefFrm = pst_handle->seq_info->maxNumRefFrm;
    _status->stSeqinitalInfo.s32AspectRateInfo = pst_handle->seq_info->aspectRateInfo;
    _status->stSeqinitalInfo.s32BitRate = pst_handle->seq_info->bitRate;
    _status->stSeqinitalInfo.s32LumaBitdepth = pst_handle->seq_info->lumaBitdepth;
    _status->stSeqinitalInfo.s32ChromaBitdepth = pst_handle->seq_info->chromaBitdepth;
    _status->stSeqinitalInfo.u8CoreIdx = pst_handle->core_idx;
}

static int flush_decode(void *param)
{
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)param;
    cviDecOnePicCfg dopc = {0};

    while(1) {
        if (cviVDecDecOnePic(pst_handle, &dopc, 0) == RETCODE_SUCCESS)
            break;
        msleep(10);
    }

    return 0;
}

int set_stride_align(void *pHandle, int align)
{
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;

    pst_handle->stride_align = align;
    return 0;
}

int set_user_pic(void *pHandle, const cviDispFrameCfg *frame_cfg)
{
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;

    memcpy(&pst_handle->usr_pic, frame_cfg, sizeof(cviDispFrameCfg));

    return 0;
}

int enable_user_pic(void *pHandle, int instant)
{
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;
    void *thread_handle;
    struct sched_param param = {
        .sched_priority = 95,
    };

    pst_handle->user_pic_enable = 1;
    pst_handle->user_pic_mode = instant;

    if (instant)
        return 0;

    //create thread to flush vcodec
    thread_handle = kthread_run(flush_decode, pst_handle, "vdec_core%d_%d_flush",
                                pst_handle->core_idx, pst_handle->channel_index);
    if (IS_ERR(thread_handle))
        return RETCODE_FAILURE;
    sched_setscheduler(thread_handle, SCHED_RR, &param);

    return 0;
}

int disable_user_pic(void *pHandle)
{
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;
    int ret;

    if (pst_handle->thread_handle != NULL) {
        pst_handle->stop_wait_interrupt = 1;
        osal_thread_join(pst_handle->thread_handle, NULL);
        pst_handle->thread_handle  = NULL;
    }

    do {
        VPU_DecFrameBufferFlush(pst_handle->handle, NULL, NULL);
        ret = VPU_DecClose(pst_handle->handle);
    } while (ret == RETCODE_VPU_STILL_RUNNING);

    ret = VPU_DecOpen(&pst_handle->handle, pst_handle->open_param);
    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "Failed to VPU_DecOpen(ret:%d)\n", ret);
        return ret;
    }

    pst_handle->stop_wait_interrupt = 0;
    pst_handle->user_pic_enable = 0;
    pst_handle->seq_status = SEQ_INIT_NON;

    return 0;
}

int set_display_mode(void *pHandle, int display_mode)
{
    DECODER_HANDLE *pst_handle = (DECODER_HANDLE *)pHandle;

    pst_handle->display_mode = display_mode;
    return 0;
}

#endif
