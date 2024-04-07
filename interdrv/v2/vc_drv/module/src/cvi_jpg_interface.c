#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include "cvi_jpg_interface.h"
#include "sophgo_jpeg_interface.h"
#include "jpulog.h"


#ifndef UNREFERENCED_PARAM
#define UNREFERENCED_PARAM(x) ((void)(x))
#endif

#define RET_JPG_TIMEOUT (-2)
#define CVI_JPG_DBG_ERR(msg, ...)
#define CVI_JPG_DBG_WARN(msg, ...)
#define CVI_JPG_DBG_INFO(msg, ...)
#define CVI_JPG_DBG_FLOW(msg, ...)
#define CVI_JPG_DBG_MEM(msg, ...)
#define CVI_JPG_DBG_IF(msg, ...)
#define CVI_JPG_DBG_LOCK(msg, ...)
#define CVI_JPG_DBG_RC(msg, ...)
#define CVI_JPG_DBG_TRACE(msg, ...)

void cviJpgGetVersion(void)
{

}

/* initial jpu core */
int CVIJpgInit(void)
{
    return sophgo_jpeg_init();
}


/* uninitial jpu core */
void CVIJpgUninit(void)
{
    sophgo_jpeg_deinit();
}

/* alloc a jpu handle for dcoder or encoder */
CVIJpgHandle CVIJpgOpen(CVIJpgConfig config)
{
    SOPHGO_S_JPEG_CONFIG stJpegConfig = {0};
    SOPHGO_JPEG_HANDLE_PTR handle;

    memset(&stJpegConfig, 0, sizeof(SOPHGO_S_JPEG_CONFIG));
    stJpegConfig.eType = config.type;
    stJpegConfig.s32Chn = config.s32ChnNum;
    if(CVIJPGCOD_ENC == config.type) {
        stJpegConfig.u.stEncConfig.eSourceFormat = config.u.enc.sourceFormat;
        stJpegConfig.u.stEncConfig.s32PicWidth = config.u.enc.picWidth;
        stJpegConfig.u.stEncConfig.s32PicHeight =  config.u.enc.picHeight;
        stJpegConfig.u.stEncConfig.s32BitstreamBufSize = config.u.enc.bitstreamBufSize;
        stJpegConfig.u.stEncConfig.bEnStuffByte = config.u.enc.bEnStuffByte;
        stJpegConfig.u.stEncConfig.eChromaInterleave = config.u.enc.chromaInterleave;
        stJpegConfig.u.stEncConfig.encHeaderMode = config.u.enc.encHeaderMode;
        stJpegConfig.u.stEncConfig.ePackedFormat = config.u.enc.packedFormat;
        stJpegConfig.u.stEncConfig.s32RotAngle = config.u.enc.rotAngle * 90;
        stJpegConfig.u.stEncConfig.s32Framerate = 0;
        stJpegConfig.u.stEncConfig.s32MirDir = config.u.enc.mirDir;
        stJpegConfig.u.stEncConfig.s32Quality = config.u.enc.quality?99: config.u.enc.quality;
        stJpegConfig.u.stEncConfig.s32SrcType = 3;
        stJpegConfig.u.stEncConfig.s32Bitrate = 10000;
        stJpegConfig.u.stEncConfig.s32OutNum = 1;
    } else {
        stJpegConfig.u.stDecConfig.stDecBuf.vbY.phys_addr = config.u.dec.dec_buf.vbY.phys_addr;
        stJpegConfig.u.stDecConfig.stDecBuf.vbCb.phys_addr = config.u.dec.dec_buf.vbCb.phys_addr;
        stJpegConfig.u.stDecConfig.stDecBuf.vbCr.phys_addr = config.u.dec.dec_buf.vbCr.phys_addr;
        stJpegConfig.u.stDecConfig.bsSize = config.u.dec.iDataLen;
        stJpegConfig.u.stDecConfig.stDecBuf.eCbcrInterleave = config.u.dec.dec_buf.chromaInterleave;
        stJpegConfig.u.stDecConfig.stDecBuf.eFormat = config.u.dec.dec_buf.format;
        stJpegConfig.u.stDecConfig.roiEnable = config.u.dec.roiEnable;
        stJpegConfig.u.stDecConfig.roiHeight = config.u.dec.roiHeight;
        stJpegConfig.u.stDecConfig.roiWidth = config.u.dec.roiWidth;
        stJpegConfig.u.stDecConfig.roiOffsetX = config.u.dec.roiOffsetX;
        stJpegConfig.u.stDecConfig.roiOffsetY = config.u.dec.roiOffsetY;
        stJpegConfig.u.stDecConfig.mirror = config.u.dec.mirDir;
        stJpegConfig.u.stDecConfig.rotation = config.u.dec.rotAngle;
        stJpegConfig.u.stDecConfig.iHorScaleMode = config.u.dec.iHorScaleMode;
        stJpegConfig.u.stDecConfig.iVerScaleMode = config.u.dec.iVerScaleMode;
    }

    handle = sophgo_jpeg_open_instance(stJpegConfig);
    if(handle == NULL) {
        JLOG(ERR, "sophgo_jpeg_open_instance open failed.\n");
        return NULL;
    }

    return handle;
}

/* close and free alloced jpu handle */
int CVIJpgClose(CVIJpgHandle jpgHandle)
{
    return sophgo_jpeg_close_instance(jpgHandle);
}

/* */
int CVIJpgGetCaps(CVIJpgHandle jpgHandle)
{
    UNREFERENCED_PARAM(jpgHandle);
    return 0;
}

/* reset jpu core */
int CVIJpgReset(CVIJpgHandle jpgHandle)
{
    UNREFERENCED_PARAM(jpgHandle);
    return 0;
}

/* flush data */
int CVIJpgFlush(CVIJpgHandle jpgHandle)
{
    int ret = 0;
    if (NULL == jpgHandle)
        return -1;

    return ret;
}

/* send jpu data to decode or encode */
int CVIJpgSendFrameData(CVIJpgHandle jpgHandle, void *data, int length,
            int s32TimeOut)
{

    return sophgo_jpeg_send_frame_data(jpgHandle, data, length, s32TimeOut);
}

/* after decoded or encoded, get data from jpu */
int CVIJpgGetFrameData(CVIJpgHandle jpgHandle, void *data, int length,
               unsigned long int *pu64HwTime)
{

    return sophgo_jpeg_get_frame_data(jpgHandle, data, length, pu64HwTime);
}

/* release stream buffer */
int CVIJpgReleaseFrameData(CVIJpgHandle jpgHandle)
{
    int ret = 0;

    ret = sophgo_jpeg_release_frame_data(jpgHandle);
    return ret;
}

/* get jpu encoder input data buffer */
int CVIJpgGetInputDataBuf(CVIJpgHandle jpgHandle, void *data, int length)
{
    int ret = 0;

    UNREFERENCED_PARAM(length);

    JLOG(INFO, "handle = %p\n", jpgHandle);

    if (NULL == jpgHandle) {
        JLOG(ERR, "jpgHandle = NULL\n");
        return -1;
    }

    sophgo_jpeg_get_input_source_data(jpgHandle, data);

    return ret;
}

int CVIVidJpuReset(void)
{
    return 0;
}

int cviJpegSetQuality(CVIJpgHandle jpgHandle, void *data)
{
    int ret = 0;

    ret = sophgo_jpeg_set_quality_params(jpgHandle, data);

    return ret;
}

int cviJpegGetQuality(CVIJpgHandle jpgHandle, void *data)
{
    int ret = 0;

    ret = sophgo_jpeg_get_quality_params(jpgHandle, data);

    return ret;
}


static int cviJpegSetChnAttr(CVIJpgHandle jpgHandle, void *arg)
{
    int ret = 0;

    return ret;
}

static int cviJpegSetMCUPerECS(CVIJpgHandle jpgHandle, void *data)
{
    int ret = 0;

    return ret;
}

int cviJpegResetChn(CVIJpgHandle jpgHandle, void *data)
{
    int ret = 0;


    return ret;
}

int cviJpegSetUserData(CVIJpgHandle jpgHandle, void *data)
{
    int ret = 0;

    return ret;
}

int cviJpegShowChnInfo(CVIJpgHandle jpgHandle, void *data)
{
    UNREFERENCED_PARAM(jpgHandle);
    UNREFERENCED_PARAM(data);

    return 0;
}

int cviJpegGetOutputFrameCount(CVIJpgHandle jpgHandle)
{
    int ret = 0;
    ret = sophgo_jpeg_get_output_count(jpgHandle);

    return ret;
}


typedef struct _CVI_JPEG_IOCTL_OP_ {
    int opNum;
    int (*ioctlFunc)(CVIJpgHandle jpgHandle, void *arg);
} CVI_JPEG_IOCTL_OP;

CVI_JPEG_IOCTL_OP cviJpegIoctlOp[] = {
    { CVI_JPEG_OP_NONE, NULL },
    { CVI_JPEG_OP_SET_QUALITY, cviJpegSetQuality },
    { CVI_JPEG_OP_GET_QUALITY, cviJpegGetQuality },
    { CVI_JPEG_OP_SET_CHN_ATTR, cviJpegSetChnAttr },
    { CVI_JPEG_OP_SET_MCUPerECS, cviJpegSetMCUPerECS },
    { CVI_JPEG_OP_RESET_CHN, cviJpegResetChn },
    { CVI_JPEG_OP_SET_USER_DATA, cviJpegSetUserData },
    { CVI_JPEG_OP_SHOW_CHN_INFO, cviJpegShowChnInfo },
};

int cviJpegIoctl(void *handle, int op, void *arg)
{
    CVIJpgHandle jpgHandle = (CVIJpgHandle)handle;
    int ret = 0;
    int currOp;

    JLOG(INFO, "\n");

    if (op <= 0 || op >= CVI_JPEG_OP_MAX) {
        JLOG(ERR, "op = %d\n", op);
        return -1;
    }

    currOp = (cviJpegIoctlOp[op].opNum & CVI_JPEG_OP_MASK) >>
         CVI_JPEG_OP_SHIFT;
    if (op != currOp) {
        JLOG(ERR, "op = %d\n", op);
        return -1;
    }

    ret = cviJpegIoctlOp[op].ioctlFunc(jpgHandle, arg);

    return ret;
}
