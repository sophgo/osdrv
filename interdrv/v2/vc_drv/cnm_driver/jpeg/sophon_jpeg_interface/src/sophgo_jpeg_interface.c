#include "sophgo_jpeg_interface.h"
#include "sophgo_jpeg_internal.h"
#include "jpuapi.h"
#include "jpulog.h"
#include "jpuapifunc.h"


typedef struct {
    void *handle;
    int isDecoder;
    unsigned s32Chn;
}SOPHGO_JPEG_CODEC_HANDLE_INFO,*SOPHGO_JPEG_CODEC_HANDLE_INFO_PTR;

static DEFINE_MUTEX(g_jpg_interface_mutex);



int sophgo_jpeg_init(void)
{
    JpgRet eRetCode = JPG_RET_SUCCESS;
    mutex_lock(&g_jpg_interface_mutex);
    eRetCode = JPU_Init();
    if (eRetCode != JPG_RET_SUCCESS && eRetCode != JPG_RET_CALLED_BEFORE) {
        JLOG(ERR, "JPU_Init failed Error code is 0x%x \n", eRetCode );
        mutex_unlock(&g_jpg_interface_mutex);
        return eRetCode;
    }

    mutex_unlock(&g_jpg_interface_mutex);

    return JPG_RET_SUCCESS;
}

void sophgo_jpeg_deinit(void)
{
    mutex_lock(&g_jpg_interface_mutex);
    JPU_DeInit();
    mutex_unlock(&g_jpg_interface_mutex);

    return;
}

SOPHGO_JPEG_HANDLE_PTR sophgo_jpeg_open_instance(SOPHGO_S_JPEG_CONFIG stJpegConfig)
{
    JpgRet ret = JPG_RET_INVALID_PARAM;
    SOPHGO_JPEG_CODEC_HANDLE_INFO *jpgCodecHandle = kzalloc(sizeof(SOPHGO_JPEG_CODEC_HANDLE_INFO), GFP_KERNEL);
    if (!jpgCodecHandle) {
        JLOG(ERR,"kzalloc fail\n");
        return NULL;
    }

    /* get new instance handle */
    if (JPGCOD_E_DEC == stJpegConfig.eType) {
        // printf("Open decoder devices!\n");
        ret = sophgo_jpeg_dec_open_instance(&jpgCodecHandle->handle, stJpegConfig.u.stDecConfig);
        if (JPG_RET_SUCCESS != ret) {
            JLOG(ERR,"Open Decode Device fail, ret %d\n", ret);
            kfree(jpgCodecHandle);
            return NULL;
        }
        jpgCodecHandle->isDecoder = TRUE;
    } else if (JPGCOD_E_ENC == stJpegConfig.eType) {
        // printf("Open encoder devices!\n");
        ret = sophgo_jpeg_enc_open_instance(&jpgCodecHandle->handle, stJpegConfig.u.stEncConfig);
        if (JPG_RET_SUCCESS != ret) {
            JLOG(ERR,"Open Encode Device fail, ret %d\n", ret);
            kfree(jpgCodecHandle);
            return NULL;
        }
        jpgCodecHandle->isDecoder = FALSE;
    }
    jpgCodecHandle->s32Chn = stJpegConfig.s32Chn;

    JLOG(INFO,"handle = %p\n", jpgCodecHandle->handle);

    return (SOPHGO_JPEG_HANDLE_PTR)jpgCodecHandle;
}
/* jpu decode and encode capacity */
int sophgo_jpeg_get_caps(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle)
{

    return 0;
}
/* close and free alloced jpu handle */
int sophgo_jpeg_close_instance(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle)
{
    int ret = JPG_RET_SUCCESS;
    SOPHGO_JPEG_CODEC_HANDLE_INFO *jpgCodecHandle;

    JpgEnterLock();
    JLOG(INFO,"handle = %p\n", pvJpgHandle);

    /* close instance handle */
    if (NULL == pvJpgHandle) {
        JpgLeaveLock();
        JLOG(ERR,"jpgHandle = NULL\n");
        return -1;
    }

    jpgCodecHandle = (SOPHGO_JPEG_CODEC_HANDLE_INFO *)pvJpgHandle;
    if (jpgCodecHandle->isDecoder) {
        ret = sophgo_jpeg_dec_close_instance(jpgCodecHandle->handle);
    } else {
        ret = sophgo_jpeg_enc_close_instance(jpgCodecHandle->handle);
    }

    kfree(pvJpgHandle);
    JpgLeaveLock();
    return ret;
}
/* reset jpu core */
int sophgo_jpeg_reset(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle)
{

    return 0;
}
/* flush data */
int sophgo_jpeg_flush(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle)
{

    return 0;
}

/* send jpu data to decode or encode */
int sophgo_jpeg_send_frame_data(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, void *pvData, int s32Length, int s32TimeOut)
{
    int ret = JPG_RET_SUCCESS;
    SOPHGO_S_FRAME_BUF_PTR pstFrameBuf;
    SOPHGO_JPEG_CODEC_HANDLE_INFO *jpgCodecHandle;

    if (NULL == pvJpgHandle) {
        JLOG(ERR,"jpgHandle = NULL\n");
        return -1;
    }

    jpgCodecHandle = (SOPHGO_JPEG_CODEC_HANDLE_INFO *)pvJpgHandle;
    do {
        if (jpgCodecHandle->isDecoder) {
            ret = sophgo_jpeg_dec_send_frame_data(jpgCodecHandle->handle, pvData, s32Length);
        } else {
            pstFrameBuf = (SOPHGO_S_FRAME_BUF_PTR)pvData;
            ret = sophgo_jpeg_enc_send_frame_data(jpgCodecHandle->handle, pstFrameBuf);
        }
    } while(0);

    return ret;
}
/* after decoded or encoded, get data from jpu */
int sophgo_jpeg_get_frame_data(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, void *pvData, int s32Length, unsigned long int *pu64HwTime)
{
    int ret = JPG_RET_SUCCESS;
    SOPHGO_JPEG_CODEC_HANDLE_INFO *jpgCodecHandle;

    if (NULL == pvJpgHandle) {
        JLOG(ERR,"jpgHandle = NULL\n");
        JpgLeaveLock();
        return -1;
    }

    jpgCodecHandle = (SOPHGO_JPEG_CODEC_HANDLE_INFO *)pvJpgHandle;
    if (jpgCodecHandle->isDecoder) {
        ret = sophgo_jpeg_dec_get_frame_data(jpgCodecHandle->handle, (SOPHGO_S_FRAME_BUF *)pvData);
    } else {
        ret = sophgo_jpeg_enc_get_stream(jpgCodecHandle->handle, pvData);
    }

    if (pu64HwTime) {
        //*pu64HwTime = pJpgInst->u64EndTime - pJpgInst->u64StartTime;
        *pu64HwTime = 0; //get the cost time
    }

    return ret;
}
/* get jpu encoder input data buffer */
int sophgo_jpeg_get_input_source_data(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, void *pvData)
{
    int ret = JPG_RET_SUCCESS;
    SOPHGO_JPEG_CODEC_HANDLE_INFO *jpgCodecHandle = (SOPHGO_JPEG_CODEC_HANDLE_INFO *)pvJpgHandle;

    JLOG(INFO,"handle = %p\n", pvJpgHandle);

    if (NULL == pvJpgHandle) {
        JLOG(INFO,"jpgHandle = NULL\n");
        JpgLeaveLock();
        return -1;
    }

    if(jpgCodecHandle->isDecoder) {
        JLOG(ERR,"do not support decode.\n");
        return JPG_RET_NOT_SUPPORT;
    } else {
        ret = sophgo_jpeg_enc_get_input_source_data(jpgCodecHandle->handle, (SOPHGO_S_FRAME_BUF *)pvData);
    }

    return ret;
}
/* release stream buffer */
int sophgo_jpeg_release_frame_data(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle)
{
    int ret = JPG_RET_SUCCESS;
    SOPHGO_JPEG_CODEC_HANDLE_INFO *jpgCodecHandle = (SOPHGO_JPEG_CODEC_HANDLE_INFO *)pvJpgHandle;

    JLOG(INFO,"handle = %p\n", pvJpgHandle);

    if (NULL == pvJpgHandle) {
        JLOG(INFO,"jpgHandle = NULL\n");
        JpgLeaveLock();
        return -1;
    }

    if(jpgCodecHandle->isDecoder) {
        ret = sophgo_jpeg_dec_release_frame_data(jpgCodecHandle->handle);
    } else {
        ret = sophgo_jpeg_enc_release_frame(jpgCodecHandle->handle);
    }

    //consider if we need a lock for each instance,and unlock it here ,lock in the sophgo_jpeg_send_frame_data

    return ret;
}

int sophgo_jpeg_set_quality_params(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, void *pVdata)
{
    int ret = JPG_RET_SUCCESS;
    SOPHGO_JPEG_CODEC_HANDLE_INFO *jpgCodecHandle;

    if (NULL == pvJpgHandle) {
        JLOG(ERR,"jpgHandle = NULL\n");
        return -1;
    }

    jpgCodecHandle = (SOPHGO_JPEG_CODEC_HANDLE_INFO *)pvJpgHandle;
    if(!jpgCodecHandle->isDecoder) {
        ret = sophgo_jpeg_enc_set_quality(jpgCodecHandle->handle, pVdata);
    }else {
        JLOG(ERR,"quality is only for encoder.\n");
        ret = JPG_RET_FAILURE;
    }

    return ret;
}

int sophgo_jpeg_get_quality_params(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle, void *pVdata)
{
    int ret = JPG_RET_SUCCESS;
    SOPHGO_JPEG_CODEC_HANDLE_INFO *jpgCodecHandle;

    if (NULL == pvJpgHandle) {
        JLOG(ERR,"jpgHandle = NULL\n");
        return -1;
    }

    jpgCodecHandle = (SOPHGO_JPEG_CODEC_HANDLE_INFO *)pvJpgHandle;
    if(!jpgCodecHandle->isDecoder) {
        ret = sophgo_jpeg_enc_get_quality(jpgCodecHandle->handle, pVdata);
    }else {
        JLOG(ERR,"quality is only for encoder.\n");
        ret = JPG_RET_FAILURE;
    }

    return ret;
}


int sophgo_jpeg_get_output_count(SOPHGO_JPEG_HANDLE_PTR pvJpgHandle)
{
    int ret = JPG_RET_SUCCESS;
    SOPHGO_JPEG_CODEC_HANDLE_INFO *jpgCodecHandle;

    if (NULL == pvJpgHandle) {
        JLOG(ERR,"jpgHandle = NULL\n");
        return -1;
    }

    jpgCodecHandle = (SOPHGO_JPEG_CODEC_HANDLE_INFO *)pvJpgHandle;
    if(!jpgCodecHandle->isDecoder) {
        ret = sophgo_jpeg_dec_get_valid_frame_count(jpgCodecHandle->handle);
    }else {
        ret = sophgo_jpeg_enc_get_valid_bitstream_count(jpgCodecHandle->handle);
    }

    return ret;
}

