//#include <linux/cvi_base_ctx.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/device.h>

//#include <base_ctx.h>
// #include "cvi_venc.h"
#include "cvi_vc_drv.h"
//#include "cvi_vc_drv_proc.h"

CVI_U32 VENC_LOG_LV = 1;

static const struct of_device_id cvi_vc_drv_match_table[] = {
    { .compatible = "cvitek,cvi_vc_drv" },
    {},
};
MODULE_DEVICE_TABLE(of, cvi_vc_drv_match_table);

wait_queue_head_t tVencWaitQueue[VENC_MAX_CHN_NUM];
static DEFINE_SPINLOCK(vc_spinlock);


module_param(VENC_LOG_LV, int, 0644);

uint32_t MaxVencChnNum = VENC_MAX_CHN_NUM;
module_param(MaxVencChnNum, uint, 0644);
#ifdef ENABLE_DEC
uint32_t MaxVdecChnNum = VDEC_MAX_CHN_NUM;
module_param(MaxVdecChnNum, uint, 0644);
#endif
bool cviRcEn = 1;
module_param(cviRcEn, bool, 0644);

struct cvi_vc_chn_info
{
    int channel_index;
    int is_encode;
    int is_jpeg;
    unsigned int ref_cnt;
    int is_channel_exist;
};

struct cvi_vc_chn_info venc_chn_info[VENC_MAX_CHN_NUM];
struct cvi_vc_chn_info vdec_chn_info[VDEC_MAX_CHN_NUM];


static uint32_t vencChnBitMap = 0;
static uint64_t vdecChnBitMap = 0;
static uint8_t  jpegEncChnBitMap[JPEG_MAX_CHN_NUM] = {0};
static uint8_t  jpegDecChnBitMap[JPEG_MAX_CHN_NUM] = {0};
static uint32_t jpegChnStart = 64;

wait_queue_head_t tVdecWaitQueue[VDEC_MAX_CHN_NUM];
static struct semaphore vencSemArry[VENC_MAX_CHN_NUM];
#ifdef ENABLE_DEC
static struct semaphore vdecSemArry[VDEC_MAX_CHN_NUM];
#endif
struct cvi_vc_drv_device *pCviVcDrvDevice;

struct clk_ctrl_info {
    int core_idx;
    int enable;
};

#ifdef VC_DRIVER_TEST
extern int jpeg_dec_test(u_long arg);
extern int jpeg_enc_test(u_long arg);
#endif

int jpeg_platform_init(struct platform_device *pdev);
void jpeg_platform_exit(void);
int vpu_drv_platform_init(struct platform_device *pdev);
int vpu_drv_platform_exit(void);
extern void cviVencDrvDeinit(void);
extern int cviVencDrvInit(void);
int cviVdecDrvInit(void);
void cviVdecDrvDeinit(void);


extern int venc_proc_init(struct device *dev);
extern int venc_proc_deinit(void);

extern int h265e_proc_init(struct device *dev);
extern int h265e_proc_deinit(void);
extern int codecinst_proc_init(struct device *dev);
extern int codecinst_proc_deinit(void);
extern int h264e_proc_init(struct device *dev);
extern int h264e_proc_deinit(void);
extern int jpege_proc_init(struct device *dev);
extern int jpege_proc_deinit(void);
extern int rc_proc_init(struct device *dev);
extern int rc_proc_deinit(void);

#ifdef ENABLE_DEC
extern int vdec_proc_init(struct device *dev);
extern int vdec_proc_deinit(void);
#endif


#if defined(CONFIG_PM)
int vpu_drv_suspend(struct platform_device *pdev, pm_message_t state);
int vpu_drv_resume(struct platform_device *pdev);
int jpeg_drv_suspend(struct platform_device *pdev, pm_message_t state);
int jpeg_drv_resume(struct platform_device *pdev);
#endif

static int cvi_vc_drv_open(struct inode *inode, struct file *filp);
static long cvi_vc_drv_venc_ioctl(struct file *filp, u_int cmd, u_long arg);
#ifdef ENABLE_DEC
static long cvi_vc_drv_vdec_ioctl(struct file *filp, u_int cmd, u_long arg);
#endif
static int cvi_vc_drv_venc_release(struct inode *inode, struct file *filp);
static int cvi_vc_drv_vdec_release(struct inode *inode, struct file *filp);

static unsigned int cvi_vc_drv_poll(struct file *filp,
                    struct poll_table_struct *wait);
static unsigned int cvi_vdec_drv_poll(struct file *filp,
                    struct poll_table_struct *wait);


//extern unsigned long vpu_get_interrupt_reason(int coreIdx);
//extern unsigned long jpu_get_interrupt_flag(int chnIdx);

const struct file_operations cvi_vc_drv_venc_fops = {
    .owner = THIS_MODULE,
    .open = cvi_vc_drv_open,
    .unlocked_ioctl = cvi_vc_drv_venc_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = cvi_vc_drv_venc_ioctl,
#endif
    .release = cvi_vc_drv_venc_release,
    .poll = cvi_vc_drv_poll,
};
#ifdef ENABLE_DEC
const struct file_operations cvi_vc_drv_vdec_fops = {
    .owner = THIS_MODULE,
    .open = cvi_vc_drv_open,
    .unlocked_ioctl = cvi_vc_drv_vdec_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = cvi_vc_drv_vdec_ioctl,
#endif
    .poll = cvi_vdec_drv_poll,
    .release = cvi_vc_drv_vdec_release,
};
#endif
static int cvi_vc_drv_open(struct inode *inode, struct file *filp)
{

    return 0;
}

static long cvi_vc_drv_venc_ioctl(struct file *filp, u_int cmd, u_long arg)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    unsigned long   flags;
    struct cvi_vc_chn_info *pstChnInfo = (struct cvi_vc_chn_info *)filp->private_data;
    int minor;
    int isJpeg = 0, i = 0;

    if(pstChnInfo)
        minor = pstChnInfo->channel_index;
    if (cmd == CVI_VC_VCODEC_SET_CHN) {
        if (copy_from_user(&minor, (int *)arg, sizeof(int)) != 0) {
            pr_err("get chn fd failed.\n");
            return s32Ret;
        }

        if (minor < 0 || minor > VENC_MAX_CHN_NUM) {
            pr_err("invalid channel index %d.\n", minor);
            return -1;
        }
        spin_lock_irqsave(&vc_spinlock, flags);
        if(!pstChnInfo) {
            pstChnInfo = &venc_chn_info[minor];
            filp->private_data = &venc_chn_info[minor];
        }
        pstChnInfo->ref_cnt++;
        pstChnInfo->channel_index = minor;
        spin_unlock_irqrestore(&vc_spinlock, flags);
        return 0;
    } else if (cmd == CVI_VC_VCODEC_GET_CHN) {
        if (copy_from_user(&isJpeg, (int *)arg, sizeof(int)) != 0) {
            pr_err("get chn fd failed.\n");
            return s32Ret;
        }
        spin_lock_irqsave(&vc_spinlock, flags);
        if (isJpeg) {
            for (i = jpegChnStart; i < JPEG_MAX_CHN_NUM; i++) {
                if (jpegEncChnBitMap[i] == 0){
                    jpegEncChnBitMap[i] = 1;
                    venc_chn_info[i].is_jpeg = true;
                    spin_unlock_irqrestore(&vc_spinlock, flags);
                    return i;
                }
            }
            spin_unlock_irqrestore(&vc_spinlock, flags);
            pr_err("get chn fd failed, not enough jpeg enc chn\n");
            return -1;
        } else {
            for (i = 0; i < VC_MAX_CHN_NUM; i++){
                if (!(vencChnBitMap & (1 << i))) {
                    vencChnBitMap |= (1 << i);
                    venc_chn_info[i].is_jpeg = false;
                    spin_unlock_irqrestore(&vc_spinlock, flags);
                    return i;
                }
            }
            spin_unlock_irqrestore(&vc_spinlock, flags);
            pr_err("get chn fd failed, not enough venc chn\n");
            return -1;
        }
    }

    if (minor < 0 || minor > VENC_MAX_CHN_NUM) {
        pr_err("invalid enc channel index %d.\n", minor);
        return -1;
    }

    if (down_interruptible(&vencSemArry[minor])) {
        return s32Ret;
    }

    switch (cmd) {
    case CVI_VC_VENC_CREATE_CHN: {
        VENC_CHN_ATTR_S stChnAttr;

        if (copy_from_user(&stChnAttr, (VENC_CHN_ATTR_S *)arg,
                   sizeof(VENC_CHN_ATTR_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_CreateChn(minor, &stChnAttr);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_CreateChn with %d\n", s32Ret);
        } else if (pstChnInfo){
            pstChnInfo->is_channel_exist = true;
        }
    } break;
    case CVI_VC_VENC_DESTROY_CHN: {
        s32Ret = CVI_VENC_DestroyChn(minor);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_DestroyChn with %d\n", s32Ret);
        } else if (pstChnInfo){
            pstChnInfo->is_channel_exist = false;
        }
    } break;
    case CVI_VC_VENC_RESET_CHN: {
        s32Ret = CVI_VENC_ResetChn(minor);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_ResetChn with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_START_RECV_FRAME: {
        VENC_RECV_PIC_PARAM_S stRecvParam;

        if (copy_from_user(&stRecvParam, (VENC_RECV_PIC_PARAM_S *)arg,
                   sizeof(VENC_RECV_PIC_PARAM_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_StartRecvFrame(minor, &stRecvParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_StartRecvFrame with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_STOP_RECV_FRAME: {
        s32Ret = CVI_VENC_StopRecvFrame(minor);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_StopRecvFrame with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_QUERY_STATUS: {
        VENC_CHN_STATUS_S stStatus;

        if (copy_from_user(&stStatus, (VENC_CHN_STATUS_S *)arg,
                   sizeof(VENC_CHN_STATUS_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_QueryStatus(minor, &stStatus);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_QueryStatus with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_CHN_STATUS_S *)arg, &stStatus,
                 sizeof(VENC_CHN_STATUS_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_CHN_ATTR: {
        VENC_CHN_ATTR_S stChnAttr;

        if (copy_from_user(&stChnAttr, (VENC_CHN_ATTR_S *)arg,
                   sizeof(VENC_CHN_ATTR_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetChnAttr(minor, &stChnAttr);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetChnAttr with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_GET_CHN_ATTR: {
        VENC_CHN_ATTR_S stChnAttr;

        s32Ret = CVI_VENC_GetChnAttr(minor, &stChnAttr);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetChnAttr with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_CHN_ATTR_S *)arg, &stChnAttr,
                 sizeof(VENC_CHN_ATTR_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_GET_STREAM: {
        VENC_STREAM_EX_S stStreamEx;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        VENC_STREAM_S stStream;
        VENC_PACK_S *pUserPack; // keep user space pointer on packs
#endif

        if (copy_from_user(&stStreamEx, (VENC_STREAM_EX_S *)arg,
                   sizeof(VENC_STREAM_EX_S)) != 0) {
            break;
        }

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        if (copy_from_user(&stStream, stStreamEx.pstStream,
                   sizeof(VENC_STREAM_S)) != 0) {
            break;
        }

        // stStream.pstPack will be replaced by kernel space packs
        // in CVI_VENC_GetStream
        pUserPack = stStream.pstPack;
        stStream.pstPack = NULL;
        s32Ret = CVI_VENC_GetStream(minor, &stStream,
                        stStreamEx.s32MilliSec);
#else
        s32Ret = CVI_VENC_GetStream(minor, stStreamEx.pstStream,
                        stStreamEx.s32MilliSec);
#endif

        if (s32Ret != CVI_SUCCESS) {
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
            if (stStream.pstPack) {
                vfree(stStream.pstPack);
                stStream.pstPack = NULL;
            }
#endif
            break;
        }

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        // copy kernel space packs to user space
        if (stStream.pstPack) {
            if (copy_to_user(pUserPack, stStream.pstPack,
                     sizeof(VENC_PACK_S) *
                     stStream.u32PackCount) != 0) {

                if (stStream.pstPack) {
                    vfree(stStream.pstPack);
                    stStream.pstPack = NULL;
                }

                s32Ret = CVI_FAILURE;
                break;
            }

            if (stStream.pstPack) {
                vfree(stStream.pstPack);
                stStream.pstPack = NULL;
            }
        }

        // restore user space pointer
        stStream.pstPack = pUserPack;
        if (copy_to_user(stStreamEx.pstStream, &stStream,
                 sizeof(VENC_STREAM_S)) != 0) {
            pr_err("%s %d failed\n", __FUNCTION__, __LINE__);
            s32Ret = CVI_FAILURE;
            break;
        }
#endif

        if (copy_to_user((VENC_STREAM_EX_S *)arg, &stStreamEx,
                 sizeof(VENC_STREAM_EX_S)) != 0) {
                 pr_err("%s %d failed\n", __FUNCTION__, __LINE__);
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_RELEASE_STREAM: {
        VENC_STREAM_S stStream;

        if (copy_from_user(&stStream, (VENC_STREAM_S *)arg,
                   sizeof(VENC_STREAM_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_ReleaseStream(minor, &stStream);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_ReleaseStream with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_INSERT_USERDATA: {
        VENC_USER_DATA_S stUserData;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        __u8 *pUserData = NULL;
#endif

        if (copy_from_user(&stUserData, (VENC_USER_DATA_S *)arg,
                   sizeof(VENC_USER_DATA_S)) != 0) {
            break;
        }

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        pUserData = vmalloc(stUserData.u32Len);
        if (pUserData == NULL) {
            s32Ret = CVI_ERR_VENC_NOMEM;
            break;
        }

        if (copy_from_user(pUserData, stUserData.pu8Data,
                   stUserData.u32Len) != 0) {
            vfree(pUserData);
            break;
        }

        stUserData.pu8Data = pUserData;
#endif

        s32Ret = CVI_VENC_InsertUserData(minor, stUserData.pu8Data,
                         stUserData.u32Len);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_InsertUserData with %d\n", s32Ret);
        }

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        if (pUserData)
            vfree(pUserData);
#endif
    } break;
    case CVI_VC_VENC_SEND_FRAME: {
        VIDEO_FRAME_INFO_EX_S stFrameEx;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        VIDEO_FRAME_INFO_S stFrame;
#endif

        if (copy_from_user(&stFrameEx, (VIDEO_FRAME_INFO_EX_S *)arg,
                   sizeof(VIDEO_FRAME_INFO_EX_S)) != 0) {
            break;
        }

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        if (copy_from_user(&stFrame, stFrameEx.pstFrame,
                   sizeof(VIDEO_FRAME_INFO_S)) != 0) {
            break;
        }
        stFrameEx.pstFrame = &stFrame;
#endif

        s32Ret = CVI_VENC_SendFrame(minor, stFrameEx.pstFrame,
                        stFrameEx.s32MilliSec);
    } break;
    case CVI_VC_VENC_SEND_FRAMEEX: {
        USER_FRAME_INFO_EX_S stUserFrameEx;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        USER_FRAME_INFO_S stUserFrameInfo;
        CVI_S32 w, h;
        __u8 *pu8QpMap = NULL;
#endif

        if (copy_from_user(&stUserFrameEx, (USER_FRAME_INFO_EX_S *)arg,
                   sizeof(USER_FRAME_INFO_EX_S)) != 0) {
            break;
        }

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        if (copy_from_user(&stUserFrameInfo, stUserFrameEx.pstUserFrame,
                   sizeof(USER_FRAME_INFO_S)) != 0) {
            break;
        }
        stUserFrameEx.pstUserFrame = &stUserFrameInfo;

        w = (((stUserFrameInfo.stUserFrame.stVFrame.u32Width + 63) & ~63) >> 6);
        h = (((stUserFrameInfo.stUserFrame.stVFrame.u32Height + 63) & ~63) >> 6);
        pu8QpMap = vmalloc(w * h);
        if (pu8QpMap == NULL) {
            s32Ret = CVI_ERR_VENC_NOMEM;
            break;
        }

        if (copy_from_user(pu8QpMap, (__u8 *)stUserFrameInfo.stUserRcInfo.u64QpMapPhyAddr,
                   w * h) != 0) {
            vfree(pu8QpMap);
            break;
        }
        stUserFrameInfo.stUserRcInfo.u64QpMapPhyAddr = (__u64)pu8QpMap;
#endif

        s32Ret = CVI_VENC_SendFrameEx(minor, stUserFrameEx.pstUserFrame,
                          stUserFrameEx.s32MilliSec);

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        if (pu8QpMap)
            vfree(pu8QpMap);
#endif
    } break;
    case CVI_VC_VENC_REQUEST_IDR: {
        CVI_BOOL bInstant;

        if (copy_from_user(&bInstant, (CVI_BOOL *)arg,
                   sizeof(CVI_BOOL)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_RequestIDR(minor, bInstant);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_RequestIDR with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_ENABLE_IDR:{
        CVI_BOOL bInstant;

        if (copy_from_user(&bInstant, (CVI_BOOL *)arg,
                   sizeof(CVI_BOOL)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_EnableIDR(minor, bInstant);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_EnableIDR with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_SET_ROI_ATTR: {
        VENC_ROI_ATTR_S stRoiAttr;

        if (copy_from_user(&stRoiAttr, (VENC_ROI_ATTR_S *)arg,
                   sizeof(VENC_ROI_ATTR_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetRoiAttr(minor, &stRoiAttr);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetRoiAttr with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_GET_ROI_ATTR: {
        VENC_ROI_ATTR_S stRoiAttr;

        if (copy_from_user(&stRoiAttr, (VENC_ROI_ATTR_S *)arg,
                   sizeof(VENC_ROI_ATTR_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_GetRoiAttr(minor, stRoiAttr.u32Index,
                         &stRoiAttr);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetRoiAttr with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_ROI_ATTR_S *)arg, &stRoiAttr,
                 sizeof(VENC_ROI_ATTR_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_H264_TRANS: {
        VENC_H264_TRANS_S stH264Trans;

        if (copy_from_user(&stH264Trans, (VENC_H264_TRANS_S *)arg,
                   sizeof(VENC_H264_TRANS_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetH264Trans(minor, &stH264Trans);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetH264Trans with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_GET_H264_TRANS: {
        VENC_H264_TRANS_S stH264Trans;

        s32Ret = CVI_VENC_GetH264Trans(minor, &stH264Trans);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetH264Trans with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_H264_TRANS_S *)arg, &stH264Trans,
                 sizeof(VENC_H264_TRANS_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_H265_PRED_UNIT: {
        VENC_H265_PU_S stH265PredUnit;

        if (copy_from_user(&stH265PredUnit,
                   (VENC_H265_PU_S *)arg,
                   sizeof(VENC_H265_PU_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetH265PredUnit(minor, &stH265PredUnit);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetH265PredUnit with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_GET_H265_PRED_UNIT: {
        VENC_H265_PU_S stH265PredUnit;

        s32Ret = CVI_VENC_GetH265PredUnit(minor, &stH265PredUnit);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetH264Entropy with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_H265_PU_S *)arg, &stH265PredUnit,
                 sizeof(VENC_H265_PU_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_H264_ENTROPY: {
        VENC_H264_ENTROPY_S stH264EntropyEnc;

        if (copy_from_user(&stH264EntropyEnc,
                   (VENC_H264_ENTROPY_S *)arg,
                   sizeof(VENC_H264_ENTROPY_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetH264Entropy(minor, &stH264EntropyEnc);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetH264Entropy with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_GET_H264_ENTROPY: {
        VENC_H264_ENTROPY_S stH264EntropyEnc;

        s32Ret = CVI_VENC_GetH264Entropy(minor, &stH264EntropyEnc);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetH264Entropy with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_H264_ENTROPY_S *)arg, &stH264EntropyEnc,
                 sizeof(VENC_H264_ENTROPY_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_H264_VUI: {
        VENC_H264_VUI_S stH264Vui;

        if (copy_from_user(&stH264Vui, (VENC_H264_VUI_S *)arg,
                   sizeof(VENC_H264_VUI_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetH264Vui(minor, &stH264Vui);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetH264Vui with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_GET_H264_VUI: {
        VENC_H264_VUI_S stH264Vui;

        s32Ret = CVI_VENC_GetH264Vui(minor, &stH264Vui);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetH264Vui with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_H264_VUI_S *)arg, &stH264Vui,
                 sizeof(VENC_H264_VUI_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_H265_VUI: {
        VENC_H265_VUI_S stH265Vui;

        if (copy_from_user(&stH265Vui, (VENC_H265_VUI_S *)arg,
                   sizeof(VENC_H265_VUI_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetH265Vui(minor, &stH265Vui);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetH265Vui with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_GET_H265_VUI: {
        VENC_H265_VUI_S stH265Vui;

        s32Ret = CVI_VENC_GetH265Vui(minor, &stH265Vui);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetH265Vui with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_H265_VUI_S *)arg, &stH265Vui,
                 sizeof(VENC_H265_VUI_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_JPEG_PARAM: {
        VENC_JPEG_PARAM_S stJpegParam;

        if (copy_from_user(&stJpegParam, (VENC_JPEG_PARAM_S *)arg,
                   sizeof(VENC_JPEG_PARAM_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetJpegParam(minor, &stJpegParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetJpegParam with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_GET_JPEG_PARAM: {
        VENC_JPEG_PARAM_S stJpegParam;

        s32Ret = CVI_VENC_GetJpegParam(minor, &stJpegParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetJpegParam with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_JPEG_PARAM_S *)arg, &stJpegParam,
                 sizeof(VENC_JPEG_PARAM_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_MJPEG_PARAM: {
        VENC_MJPEG_PARAM_S stMjpegParam;
        if (copy_from_user(&stMjpegParam, (VENC_MJPEG_PARAM_S *) arg,
            sizeof(VENC_MJPEG_PARAM_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetMjpegParam(minor, &stMjpegParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetMjpegParam with:%d\n", s32Ret);
            break;
        }
    } break;

    case CVI_VC_VENC_GET_MJPEG_PARAM: {
        VENC_MJPEG_PARAM_S stMjpegParam;

        s32Ret = CVI_VENC_GetMjpegParam(minor, &stMjpegParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetMjpegParam with:%d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_MJPEG_PARAM_S *)arg, &stMjpegParam,
                sizeof(VENC_MJPEG_PARAM_S)) != 0) {
            pr_err("CVI_VENC_GetMjpegParam with:%d", s32Ret);
            break;
        }
    } break;
    case CVI_VC_VENC_GET_RC_PARAM: {
        VENC_RC_PARAM_S stRcParam;

        s32Ret = CVI_VENC_GetRcParam(minor, &stRcParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetRcParam with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_RC_PARAM_S *)arg, &stRcParam,
                 sizeof(VENC_RC_PARAM_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_RC_PARAM: {
        VENC_RC_PARAM_S stRcParam;

        if (copy_from_user(&stRcParam, (VENC_RC_PARAM_S *)arg,
                   sizeof(VENC_RC_PARAM_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetRcParam(minor, &stRcParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetRcParam with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_SET_REF_PARAM: {
        VENC_REF_PARAM_S stRefParam;

        if (copy_from_user(&stRefParam, (VENC_REF_PARAM_S *)arg,
                   sizeof(VENC_REF_PARAM_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetRefParam(minor, &stRefParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetRefParam with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_GET_REF_PARAM: {
        VENC_REF_PARAM_S stRefParam;

        s32Ret = CVI_VENC_GetRefParam(minor, &stRefParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetRefParam with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_REF_PARAM_S *)arg, &stRefParam,
                 sizeof(VENC_REF_PARAM_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_H265_TRANS: {
        VENC_H265_TRANS_S stH265Trans;

        if (copy_from_user(&stH265Trans, (VENC_H265_TRANS_S *)arg,
                   sizeof(VENC_H265_TRANS_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetH265Trans(minor, &stH265Trans);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetH265Trans with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_GET_H265_TRANS: {
        VENC_H265_TRANS_S stH265Trans;

        s32Ret = CVI_VENC_GetH265Trans(minor, &stH265Trans);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetH265Trans with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_H265_TRANS_S *)arg, &stH265Trans,
                 sizeof(VENC_H265_TRANS_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_FRAMELOST_STRATEGY: {
        VENC_FRAMELOST_S stFrmLostParam;

        if (copy_from_user(&stFrmLostParam, (VENC_FRAMELOST_S *)arg,
                   sizeof(VENC_FRAMELOST_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetFrameLostStrategy(minor, &stFrmLostParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetFrameLostStrategy with %d\n",
                   s32Ret);
        }
    } break;
    case CVI_VC_VENC_GET_FRAMELOST_STRATEGY: {
        VENC_FRAMELOST_S stFrmLostParam;

        s32Ret = CVI_VENC_GetFrameLostStrategy(minor, &stFrmLostParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetFrameLostStrategy with %d\n",
                   s32Ret);
            break;
        }

        if (copy_to_user((VENC_FRAMELOST_S *)arg, &stFrmLostParam,
                 sizeof(VENC_FRAMELOST_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_SUPERFRAME_STRATEGY: {
        VENC_SUPERFRAME_CFG_S stSuperFrmParam;

        if (copy_from_user(&stSuperFrmParam,
                   (VENC_SUPERFRAME_CFG_S *)arg,
                   sizeof(VENC_SUPERFRAME_CFG_S)) != 0) {
            break;
        }

        s32Ret =
            CVI_VENC_SetSuperFrameStrategy(minor, &stSuperFrmParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetSuperFrameStrategy with %d\n",
                   s32Ret);
        }
    } break;
    case CVI_VC_VENC_GET_SUPERFRAME_STRATEGY: {
        VENC_SUPERFRAME_CFG_S stSuperFrmParam;

        s32Ret =
            CVI_VENC_GetSuperFrameStrategy(minor, &stSuperFrmParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetSuperFrameStrategy with %d\n",
                   s32Ret);
            break;
        }

        if (copy_to_user((VENC_SUPERFRAME_CFG_S *)arg, &stSuperFrmParam,
                 sizeof(VENC_SUPERFRAME_CFG_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_CHN_PARAM: {
        VENC_CHN_PARAM_S stChnParam;

        if (copy_from_user(&stChnParam, (VENC_CHN_PARAM_S *)arg,
                   sizeof(VENC_CHN_PARAM_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetChnParam(minor, &stChnParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetChnParam with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_GET_CHN_PARAM: {
        VENC_CHN_PARAM_S stChnParam;

        s32Ret = CVI_VENC_GetChnParam(minor, &stChnParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetChnParam with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_CHN_PARAM_S *)arg, &stChnParam,
                 sizeof(VENC_CHN_PARAM_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_MOD_PARAM: {
        VENC_PARAM_MOD_S stModParam;

        if (copy_from_user(&stModParam, (VENC_PARAM_MOD_S *)arg,
                   sizeof(VENC_PARAM_MOD_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetModParam(&stModParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetModParam with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_GET_MOD_PARAM: {
        VENC_PARAM_MOD_S stModParam;

        if (copy_from_user(&stModParam, (VENC_PARAM_MOD_S *)arg,
                   sizeof(VENC_PARAM_MOD_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_GetModParam(&stModParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetModParam with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_PARAM_MOD_S *)arg, &stModParam,
                 sizeof(VENC_PARAM_MOD_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_ATTACH_VBPOOL: {
        VENC_CHN_POOL_S stPool;

        if (copy_from_user(&stPool, (VENC_CHN_POOL_S *)arg,
                   sizeof(VENC_CHN_POOL_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_AttachVbPool(minor, &stPool);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_AttachVbPool with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_DETACH_VBPOOL: {
        s32Ret = CVI_VENC_DetachVbPool(minor);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_DetachVbPool with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_SET_CUPREDICTION: {
        VENC_CU_PREDICTION_S stCuPrediction;

        if (copy_from_user(&stCuPrediction, (VENC_CU_PREDICTION_S *)arg,
                   sizeof(VENC_CU_PREDICTION_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetCuPrediction(minor, &stCuPrediction);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetCuPrediction with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_GET_CUPREDICTION: {
        VENC_CU_PREDICTION_S stCuPrediction;

        s32Ret = CVI_VENC_GetCuPrediction(minor, &stCuPrediction);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetCuPrediction with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_CU_PREDICTION_S *)arg, &stCuPrediction,
                 sizeof(VENC_CU_PREDICTION_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_CALC_FRAME_PARAM: {
        VENC_FRAME_PARAM_S stFrameParam;

        if (copy_from_user(&stFrameParam, (VENC_FRAME_PARAM_S *)arg,
                   sizeof(VENC_FRAME_PARAM_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_CalcFrameParam(minor, &stFrameParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_CalcFrameParam with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_SET_FRAME_PARAM: {
        VENC_FRAME_PARAM_S stFrameParam;

        if (copy_from_user(&stFrameParam, (VENC_FRAME_PARAM_S *)arg,
                   sizeof(VENC_FRAME_PARAM_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetFrameParam(minor, &stFrameParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetFrameParam with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_GET_FRAME_PARAM: {
        VENC_FRAME_PARAM_S stFrameParam;

        s32Ret = CVI_VENC_GetFrameParam(minor, &stFrameParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetFrameParam with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_FRAME_PARAM_S *)arg, &stFrameParam,
                 sizeof(VENC_FRAME_PARAM_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_H264_SLICE_SPLIT: {
        VENC_H264_SLICE_SPLIT_S stH264Split;

        if (copy_from_user(&stH264Split, (VENC_H264_SLICE_SPLIT_S *)arg,
                sizeof(VENC_H264_SLICE_SPLIT_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetH264SliceSplit(minor, &stH264Split);
    } break;
    case CVI_VC_VENC_GET_H264_SLICE_SPLIT: {
        VENC_H264_SLICE_SPLIT_S stH264Split;

        s32Ret = CVI_VENC_GetH264SliceSplit(minor, &stH264Split);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetH264SliceSplit with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_H264_SLICE_SPLIT_S *)arg, &stH264Split,
                 sizeof(VENC_H264_SLICE_SPLIT_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_H265_SLICE_SPLIT: {
        VENC_H265_SLICE_SPLIT_S stH265Split;

        if (copy_from_user(&stH265Split, (VENC_H265_SLICE_SPLIT_S *)arg,
                sizeof(VENC_H265_SLICE_SPLIT_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetH265SliceSplit(minor, &stH265Split);
    } break;
    case CVI_VC_VENC_GET_H265_SLICE_SPLIT: {
        VENC_H265_SLICE_SPLIT_S stH265Split;

        s32Ret = CVI_VENC_GetH265SliceSplit(minor, &stH265Split);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetH265SliceSplit with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_H265_SLICE_SPLIT_S *)arg, &stH265Split,
                 sizeof(VENC_H265_SLICE_SPLIT_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_H264_Dblk: {
        VENC_H264_DBLK_S stH264dblk;

        if (copy_from_user(&stH264dblk, (VENC_H264_DBLK_S *)arg,
                   sizeof(VENC_H264_DBLK_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetH264Dblk(minor, &stH264dblk);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetH264Dblk with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_GET_H264_Dblk: {
        VENC_H264_DBLK_S stH264dblk;

        s32Ret = CVI_VENC_GetH264Dblk(minor, &stH264dblk);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetH264Dblk with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_H264_DBLK_S *)arg, &stH264dblk,
                 sizeof(VENC_H264_DBLK_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_H265_Dblk: {
        VENC_H265_DBLK_S stH265dblk;

        if (copy_from_user(&stH265dblk, (VENC_H265_DBLK_S *)arg,
                   sizeof(VENC_H265_DBLK_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetH265Dblk(minor, &stH265dblk);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetH265Dblk with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_GET_H265_Dblk: {
        VENC_H265_DBLK_S stH265dblk;

        s32Ret = CVI_VENC_GetH265Dblk(minor, &stH265dblk);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetH265Dblk with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_H265_DBLK_S *)arg, &stH265dblk,
                 sizeof(VENC_H265_DBLK_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_H264_INTRA_PRED: {
        VENC_H264_INTRA_PRED_S stH264IntraPred;

        if (copy_from_user(&stH264IntraPred, (VENC_H264_INTRA_PRED_S *)arg,
                sizeof(VENC_H264_INTRA_PRED_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetH264IntraPred(minor, &stH264IntraPred);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetH264IntraPred with %d\n", s32Ret);
            break;
        }
    } break;
    case CVI_VC_VENC_GET_H264_INTRA_PRED: {
        VENC_H264_INTRA_PRED_S stH264IntraPred;

        s32Ret = CVI_VENC_GetH264IntraPred(minor, &stH264IntraPred);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetH264IntraPred with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_H264_INTRA_PRED_S *)arg, &stH264IntraPred,
                 sizeof(VENC_H264_INTRA_PRED_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_CUSTOM_MAP: {
        VENC_CUSTOM_MAP_S stVeCustomMap;

        if (copy_from_user(&stVeCustomMap, (VENC_CUSTOM_MAP_S *)arg,
                sizeof(VENC_CUSTOM_MAP_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetCustomMap(minor, &stVeCustomMap);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetCustomMap with %d\n", s32Ret);
            break;
        }
    } break;
    case CVI_VC_VENC_GET_INTINAL_INFO: {
        VENC_INITIAL_INFO_S stVencInitialInfo;

        s32Ret = CVI_VENC_GetIntialInfo(minor, &stVencInitialInfo);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetIntialInfo with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_INITIAL_INFO_S *)arg, &stVencInitialInfo,
                 sizeof(VENC_INITIAL_INFO_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_SET_H265_SAO: {
        VENC_H265_SAO_S stH265Sao;

        if (copy_from_user(&stH265Sao, (VENC_H265_SAO_S *)arg,
                   sizeof(VENC_H265_SAO_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetH265Sao(minor, &stH265Sao);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetH265Sao with %d\n", s32Ret);
        }

    } break;
    case CVI_VC_VENC_GET_H265_SAO: {
        VENC_H265_SAO_S stH265Sao;

        s32Ret = CVI_VENC_GetH265Sao(minor, &stH265Sao);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetH265Sao with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_H265_SAO_S *)arg, &stH265Sao,
                 sizeof(VENC_H265_SAO_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VENC_GET_HEADER: {
        VENC_ENCODE_HEADER_S stEncodeHeader;
        if (copy_from_user(&stEncodeHeader, (VENC_ENCODE_HEADER_S *)arg,
                   sizeof(VENC_ENCODE_HEADER_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_GetEncodeHeader(minor, &stEncodeHeader);
        if (s32Ret != CVI_SUCCESS) {
            break;
        }

        if (copy_to_user((VENC_ENCODE_HEADER_S *)arg, &stEncodeHeader,
            sizeof(VENC_ENCODE_HEADER_S)) != 0) {
            pr_err("%s %d failed\n", __FUNCTION__, __LINE__);
            s32Ret = CVI_FAILURE;
        }
    } break;

    case CVI_VC_VENC_GET_EXT_ADDR: {
        extern int vdi_get_ddr_map(unsigned long core_idx);
        int ext_addr = vdi_get_ddr_map(0);

        if (copy_to_user((int *)arg, &ext_addr, sizeof(int)) != 0) {
            pr_err("%s %d failed\n", __FUNCTION__, __LINE__);
            s32Ret = CVI_FAILURE;
        }
    } break;

    case CVI_VC_VENC_SET_SEARCH_WINDOW: {
        VENC_SEARCH_WINDOW_S stVencSearchWidow;

        if (copy_from_user(&stVencSearchWidow,
                   (VENC_SEARCH_WINDOW_S *)arg,
                   sizeof(VENC_SEARCH_WINDOW_S)) != 0) {
            break;
        }

        s32Ret = CVI_VENC_SetSearchWindow(minor, &stVencSearchWidow);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_SetSearchWindow with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VENC_GET_SEARCH_WINDOW: {
        VENC_SEARCH_WINDOW_S stVencSearchWidow;

        s32Ret = CVI_VENC_GetSearchWindow(minor, &stVencSearchWidow);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VENC_GetSearchWindow with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VENC_SEARCH_WINDOW_S *)arg, &stVencSearchWidow,
                 sizeof(VENC_SEARCH_WINDOW_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;


#ifdef VC_DRIVER_TEST
    case CVI_VC_VENC_ENC_JPEG_TEST: {
        s32Ret = jpeg_enc_test(arg);
    } break;
#endif
    default: {
        pr_err("venc un-handle cmd id: %x\n", cmd);
    } break;
    }

    up(&vencSemArry[minor]);
    return s32Ret;
}
#ifdef ENABLE_DEC
static long cvi_vc_drv_vdec_ioctl(struct file *filp, u_int cmd, u_long arg)
{
    CVI_S32 s32Ret = CVI_FAILURE;
    unsigned long   flags;
    struct cvi_vc_chn_info *pstChnInfo = (struct cvi_vc_chn_info *)filp->private_data;
    unsigned int minor;
    int isJpeg = 0, i = 0;

    if(pstChnInfo)
       minor = pstChnInfo->channel_index;
    if(cmd == CVI_VC_VCODEC_SET_CHN) {
        if (copy_from_user(&minor, (int *)arg, sizeof(int)) != 0) {
            pr_err("get chn fd failed.\n");
            return s32Ret;
        }

        if(minor < 0 || minor > VDEC_MAX_CHN_NUM) {
            pr_err("invalid channel index %d.\n", minor);
            return -1;
        }
        spin_lock_irqsave(&vc_spinlock, flags);
        if(!pstChnInfo) {
            pstChnInfo = &vdec_chn_info[minor];
            filp->private_data = &vdec_chn_info[minor];
        }
        pstChnInfo->ref_cnt++;
        pstChnInfo->channel_index = minor;
        spin_unlock_irqrestore(&vc_spinlock, flags);

        return 0;
    } else if (cmd == CVI_VC_VCODEC_GET_CHN) {
        if (copy_from_user(&isJpeg, (int *)arg, sizeof(int)) != 0) {
            pr_err("get chn fd failed.\n");
            return s32Ret;
        }
        spin_lock_irqsave(&vc_spinlock, flags);
        if (isJpeg) {
            for (i = jpegChnStart; i < JPEG_MAX_CHN_NUM; i++) {
                if (jpegDecChnBitMap[i] == 0){
                    jpegDecChnBitMap[i] = 1;
                    vdec_chn_info[i].is_jpeg = true;
                    spin_unlock_irqrestore(&vc_spinlock, flags);
                    return i;
                }
            }
            spin_unlock_irqrestore(&vc_spinlock, flags);
            pr_err("get chn fd failed, not enough jpeg dec chn\n");
            return -1;
        } else {
            for (i = 0; i < VC_MAX_CHN_NUM*2; i++){
                if (!(vdecChnBitMap & ((uint64_t)1 << i))) {
                    vdecChnBitMap |= ((uint64_t)1 << i);
                    vdec_chn_info[i].is_jpeg = false;
                    spin_unlock_irqrestore(&vc_spinlock, flags);
                    return i;
                }
            }
            spin_unlock_irqrestore(&vc_spinlock, flags);
            pr_err("get chn fd failed, not enough vdec chn\n");
            return -1;
        }
    }

    if (minor < 0 || minor > VDEC_MAX_CHN_NUM) {
        pr_err("invalid dec channel index %d.\n", minor);
        return -1;
    }

    if (down_interruptible(&vdecSemArry[minor])) {
        return s32Ret;
    }

    switch (cmd) {
    case CVI_VC_VDEC_CREATE_CHN: {
        VDEC_CHN_ATTR_S stChnAttr;

        if (copy_from_user(&stChnAttr, (VDEC_CHN_ATTR_S *)arg,
                   sizeof(VDEC_CHN_ATTR_S)) != 0) {
            break;
        }

        s32Ret = CVI_VDEC_CreateChn(minor, &stChnAttr);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VDEC_CreateChn with %d\n", s32Ret);
        } else if (pstChnInfo){
            pstChnInfo->is_channel_exist = true;
        }
    } break;
    case CVI_VC_VDEC_DESTROY_CHN: {
        s32Ret = CVI_VDEC_DestroyChn(minor);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VDEC_DestroyChn with %d\n", s32Ret);
        } else if (pstChnInfo){
            pstChnInfo->is_channel_exist = false;
        }
    } break;
    case CVI_VC_VDEC_GET_CHN_ATTR: {
        VDEC_CHN_ATTR_S stChnAttr;

        s32Ret = CVI_VDEC_GetChnAttr(minor, &stChnAttr);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VDEC_GetChnAttr with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VDEC_CHN_ATTR_S *)arg, &stChnAttr,
                 sizeof(VDEC_CHN_ATTR_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VDEC_SET_CHN_ATTR: {
        VDEC_CHN_ATTR_S stChnAttr;

        if (copy_from_user(&stChnAttr, (VDEC_CHN_ATTR_S *)arg,
                   sizeof(VDEC_CHN_ATTR_S)) != 0) {
            break;
        }

        s32Ret = CVI_VDEC_SetChnAttr(minor, &stChnAttr);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VDEC_SetChnAttr with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VDEC_START_RECV_STREAM: {
        s32Ret = CVI_VDEC_StartRecvStream(minor);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VDEC_StartRecvStream with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VDEC_STOP_RECV_STREAM: {
        s32Ret = CVI_VDEC_StopRecvStream(minor);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VDEC_StopRecvStream with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VDEC_QUERY_STATUS: {
        VDEC_CHN_STATUS_S stStatus;

        if (copy_from_user(&stStatus, (VDEC_CHN_STATUS_S *)arg,
                   sizeof(VDEC_CHN_STATUS_S)) != 0) {
            break;
        }

        s32Ret = CVI_VDEC_QueryStatus(minor, &stStatus);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VDEC_QueryStatus with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VDEC_CHN_STATUS_S *)arg, &stStatus,
                 sizeof(VDEC_CHN_STATUS_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VDEC_RESET_CHN: {
        s32Ret = CVI_VDEC_ResetChn(minor);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VDEC_ResetChn with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VDEC_SET_CHN_PARAM: {
        VDEC_CHN_PARAM_S stChnParam;

        if (copy_from_user(&stChnParam, (VDEC_CHN_PARAM_S *)arg,
                   sizeof(VDEC_CHN_PARAM_S)) != 0) {
            break;
        }

        s32Ret = CVI_VDEC_SetChnParam(minor, &stChnParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VDEC_SetChnParam with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VDEC_GET_CHN_PARAM: {
        VDEC_CHN_PARAM_S stChnParam;

        s32Ret = CVI_VDEC_GetChnParam(minor, &stChnParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VDEC_GetChnParam with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VDEC_CHN_PARAM_S *)arg, &stChnParam,
                 sizeof(VDEC_CHN_PARAM_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VDEC_SEND_STREAM: {
        VDEC_STREAM_EX_S stStreamEx;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        VDEC_STREAM_S stStream;
        __u8 *pStreamData = NULL;
#endif

        if (copy_from_user(&stStreamEx, (VDEC_STREAM_EX_S *)arg,
                   sizeof(VDEC_STREAM_EX_S)) != 0) {
            break;
        }

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        if (copy_from_user(&stStream, stStreamEx.pstStream,
                   sizeof(VDEC_STREAM_S)) != 0) {
            break;
        }
        stStreamEx.pstStream = &stStream;

        if (stStream.u32Len) {
            pStreamData = vmalloc(stStream.u32Len);
            if (pStreamData == NULL) {
                s32Ret = CVI_ERR_VENC_NOMEM;
                break;
            }

            if (copy_from_user(pStreamData, stStream.pu8Addr,
                       stStream.u32Len) != 0) {
                vfree(pStreamData);
                break;
            }
        }
        stStream.pu8Addr = pStreamData;
#endif

        s32Ret = CVI_VDEC_SendStream(minor, stStreamEx.pstStream,
                         stStreamEx.s32MilliSec);
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        if (pStreamData)
            vfree(pStreamData);
#endif
    } break;
    case CVI_VC_VDEC_GET_FRAME: {
        VIDEO_FRAME_INFO_EX_S stFrameInfoEx;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        VIDEO_FRAME_INFO_S stFrameInfo;
        VIDEO_FRAME_INFO_S *pUserFrameInfo; // keep user space pointer on frame info
#endif

        if (copy_from_user(&stFrameInfoEx, (VIDEO_FRAME_INFO_EX_S *)arg,
                   sizeof(VIDEO_FRAME_INFO_EX_S)) != 0) {
            break;
        }

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        if (copy_from_user(&stFrameInfo, stFrameInfoEx.pstFrame,
                   sizeof(VIDEO_FRAME_INFO_S)) != 0) {
            break;
        }
        pUserFrameInfo = stFrameInfoEx.pstFrame;
        stFrameInfoEx.pstFrame = &stFrameInfo; // replace user space pointer
#endif

        s32Ret = CVI_VDEC_GetFrame(minor, stFrameInfoEx.pstFrame,
                       stFrameInfoEx.s32MilliSec);
        if (s32Ret != CVI_SUCCESS) {
            break;
        }

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        if (copy_to_user(pUserFrameInfo, stFrameInfoEx.pstFrame,
                   sizeof(VIDEO_FRAME_INFO_S)) != 0) {
            break;
        }
        stFrameInfoEx.pstFrame = pUserFrameInfo; // restore user space pointer
#endif

        if (copy_to_user((VIDEO_FRAME_INFO_EX_S *)arg, &stFrameInfoEx,
                 sizeof(VIDEO_FRAME_INFO_EX_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
    case CVI_VC_VDEC_RELEASE_FRAME: {
        VIDEO_FRAME_INFO_S stFrameInfo;

        if (copy_from_user(&stFrameInfo, (VIDEO_FRAME_INFO_S *)arg,
                   sizeof(VIDEO_FRAME_INFO_S)) != 0) {
            break;
        }

        s32Ret = CVI_VDEC_ReleaseFrame(minor, &stFrameInfo);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VDEC_ReleaseFrame with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VDEC_ATTACH_VBPOOL: {
        VDEC_CHN_POOL_S stPool;

        if (copy_from_user(&stPool, (VDEC_CHN_POOL_S *)arg,
                   sizeof(VDEC_CHN_POOL_S)) != 0) {
            break;
        }

        s32Ret = CVI_VDEC_AttachVbPool(minor, &stPool);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VDEC_AttachVbPool with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VDEC_DETACH_VBPOOL: {
        s32Ret = CVI_VDEC_DetachVbPool(minor);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VDEC_DetachVbPool with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VDEC_SET_MOD_PARAM: {
        VDEC_MOD_PARAM_S stModParam;

        if (copy_from_user(&stModParam, (VDEC_MOD_PARAM_S *)arg,
                   sizeof(VDEC_MOD_PARAM_S)) != 0) {
            break;
        }

        s32Ret = CVI_VDEC_SetModParam(&stModParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VDEC_SetModParam with %d\n", s32Ret);
        }
    } break;
    case CVI_VC_VDEC_GET_MOD_PARAM: {
        VDEC_MOD_PARAM_S stModParam;

        if (copy_from_user(&stModParam, (VDEC_MOD_PARAM_S *)arg,
                   sizeof(VDEC_MOD_PARAM_S)) != 0) {
            break;
        }

        s32Ret = CVI_VDEC_GetModParam(&stModParam);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("CVI_VDEC_GetModParam with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((VDEC_MOD_PARAM_S *)arg, &stModParam,
                 sizeof(VDEC_MOD_PARAM_S)) != 0) {
            s32Ret = CVI_FAILURE;
        }
    } break;
#ifdef VC_DRIVER_TEST
    case CVI_VC_VDEC_DEC_JPEG_TEST: {
        s32Ret = jpeg_dec_test(arg);
    } break;
#endif
    case CVI_VC_VDEC_FRAME_ADD_USER: {
        VIDEO_FRAME_INFO_S stFrameInfo;

        if (copy_from_user(&stFrameInfo, (VIDEO_FRAME_INFO_S *)arg, sizeof(VIDEO_FRAME_INFO_S)) != 0) {
            break;
        }

        s32Ret = CVI_VDEC_FrameBufferAddUser(minor, &stFrameInfo);
    }break;

    case CVI_VC_VDEC_SET_STRIDE_ALIGN: {
        CVI_U32 align;

        if (copy_from_user(&align, (CVI_U32 *)arg, sizeof(CVI_U32)) != 0) {
            break;
        }

        s32Ret = drv_vdec_set_stride_align(minor, align);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("drv_vdec_set_stride_align with %d\n", s32Ret);
            break;
        }
    } break;

    case CVI_VC_VDEC_SET_USER_PIC: {
        VIDEO_FRAME_INFO_S usr_pic;

        if (copy_from_user(&usr_pic, (VIDEO_FRAME_INFO_S *)arg,
                   sizeof(VIDEO_FRAME_INFO_S)) != 0) {
            break;
        }

        s32Ret = drv_vdec_set_user_pic(minor, &usr_pic);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("drv_vdec_set_user_pic with %d\n", s32Ret);
            break;
        }
    } break;

    case CVI_VC_VDEC_ENABLE_USER_PIC: {
        CVI_BOOL instant;

        if (copy_from_user(&instant, (CVI_BOOL *)arg, sizeof(CVI_BOOL)) != 0) {
            break;
        }

        s32Ret = drv_vdec_enable_user_pic(minor, instant);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("drv_vdec_enable_user_pic with %d\n", s32Ret);
            break;
        }
    } break;

    case CVI_VC_VDEC_DISABLE_USER_PIC: {
        s32Ret = drv_vdec_disable_user_pic(minor);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("drv_vdec_disable_user_pic with %d\n", s32Ret);
            break;
        }
    } break;

    case CVI_VC_VDEC_SET_DISPLAY_MODE: {
        VIDEO_DISPLAY_MODE_E display_mode;

        if (copy_from_user(&display_mode, (VIDEO_DISPLAY_MODE_E *)arg,
            sizeof(VIDEO_DISPLAY_MODE_E)) != 0) {
            break;
        }

        s32Ret = drv_vdec_set_display_mode(minor, display_mode);
        if (s32Ret != CVI_SUCCESS) {
            pr_err("drv_vdec_set_display_mode with %d\n", s32Ret);
            break;
        }
    } break;

    default: {
        pr_err("vdec un-handle cmd id: %x\n", cmd);
    } break;
    }

    up(&vdecSemArry[minor]);
    return s32Ret;
}
#endif
extern CVI_S32 cviGetLeftStreamFrames(CVI_S32 VeChn);
CVI_S32 cviVdecGetOutputFrameCount(VDEC_CHN VdChn);

static unsigned int cvi_vc_drv_poll(struct file *filp,
                    struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    unsigned int minor = -1;
    struct cvi_vc_chn_info *pstChnInfo = (struct cvi_vc_chn_info *)filp->private_data;

    if(pstChnInfo && pstChnInfo->channel_index >= 0) {
        minor = pstChnInfo->channel_index;
        poll_wait(filp, &tVencWaitQueue[minor], wait);

        if(cviGetLeftStreamFrames(minor) > 0) {
            mask |= POLLIN | POLLRDNORM;
        }
    }

    return mask;
}


static unsigned int cvi_vdec_drv_poll(struct file *filp,
                    struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    unsigned int minor = -1;
    struct cvi_vc_chn_info *pstChnInfo = (struct cvi_vc_chn_info *)filp->private_data;

    if(pstChnInfo && pstChnInfo->channel_index >= 0) {
        minor = pstChnInfo->channel_index;
        poll_wait(filp, &tVdecWaitQueue[minor], wait);

        if(cviVdecGetOutputFrameCount(minor) > 0) {
            mask |= POLLIN | POLLRDNORM;
        }
    }

    return mask;
}

static int cvi_vc_drv_venc_release(struct inode *inode, struct file *filp)
{
    struct cvi_vc_chn_info *pstChnInfo = (struct cvi_vc_chn_info *)filp->private_data;
    unsigned long   flags;

    if(pstChnInfo && pstChnInfo->channel_index >= 0) {
        spin_lock_irqsave(&vc_spinlock, flags);
        pstChnInfo->ref_cnt--;
        spin_unlock_irqrestore(&vc_spinlock, flags);
        if(!pstChnInfo->ref_cnt) {
            if(pstChnInfo->is_channel_exist) {
                CVI_VENC_StopRecvFrame(pstChnInfo->channel_index);
                CVI_VENC_DestroyChn(pstChnInfo->channel_index);
                pstChnInfo->is_channel_exist = false;
            }
            spin_lock_irqsave(&vc_spinlock, flags);
            if (pstChnInfo->is_jpeg) {
                jpegEncChnBitMap[pstChnInfo->channel_index] = 0;
                pstChnInfo->is_jpeg = false;
            } else {
                vencChnBitMap &= ~(1<<pstChnInfo->channel_index);
            }
            spin_unlock_irqrestore(&vc_spinlock, flags);
            filp->private_data = NULL;
        }
    }

    return 0;
}

static int cvi_vc_drv_vdec_release(struct inode *inode, struct file *filp)
{
    struct cvi_vc_chn_info *pstChnInfo = (struct cvi_vc_chn_info *)filp->private_data;
    unsigned long   flags;

    if(pstChnInfo && pstChnInfo->channel_index >= 0) {
        spin_lock_irqsave(&vc_spinlock, flags);
        pstChnInfo->ref_cnt--;
        spin_unlock_irqrestore(&vc_spinlock, flags);
        if(!pstChnInfo->ref_cnt) {
            if(pstChnInfo->is_channel_exist) {
                CVI_VDEC_DestroyChn(pstChnInfo->channel_index);
                pstChnInfo->is_channel_exist = false;
            }
            spin_lock_irqsave(&vc_spinlock, flags);
            if (pstChnInfo->is_jpeg) {
                jpegDecChnBitMap[pstChnInfo->channel_index] = 0;
                pstChnInfo->is_jpeg = false;
            } else {
                vdecChnBitMap &= ~((uint64_t)1<<pstChnInfo->channel_index);
            }
            spin_unlock_irqrestore(&vc_spinlock, flags);
            filp->private_data = NULL;
        }
    }

    return 0;
}


static int cvi_vc_drv_register_cdev(struct cvi_vc_drv_device *vdev)
{
    int err = 0;
    int i = 0;

    vdev->cvi_vc_class = class_create(THIS_MODULE, CVI_VC_DRV_CLASS_NAME);
    if (IS_ERR(vdev->cvi_vc_class)) {
        pr_err("create class failed\n");
        return PTR_ERR(vdev->cvi_vc_class);
    }

    /* get the major number of the character device */
    if ((alloc_chrdev_region(&vdev->venc_cdev_id, 0, 1,
                 CVI_VC_DRV_ENCODER_DEV_NAME)) < 0) {
        err = -EBUSY;
        pr_err("could not allocate major number\n");
        return err;
    }
    vdev->s_venc_major = MAJOR(vdev->venc_cdev_id);
    vdev->p_venc_cdev = vzalloc(sizeof(struct cdev));

    {
        dev_t subDevice;
        subDevice = MKDEV(vdev->s_venc_major, 0);

        /* initialize the device structure and register the device with the kernel */
        cdev_init(vdev->p_venc_cdev, &cvi_vc_drv_venc_fops);
        vdev->p_venc_cdev->owner = THIS_MODULE;

        if ((cdev_add(vdev->p_venc_cdev, subDevice, 1)) < 0) {
            err = -EBUSY;
            pr_err("could not allocate chrdev\n");
            return err;
        }

        device_create(vdev->cvi_vc_class, NULL, subDevice, NULL, "%s",
                  CVI_VC_DRV_ENCODER_DEV_NAME);
        for( i = 0; i <VENC_MAX_CHN_NUM; i++) {
            init_waitqueue_head(&tVencWaitQueue[i]);
            sema_init(&vencSemArry[i], 1);
        }
        memset(venc_chn_info, 0, sizeof(venc_chn_info));
    }
#ifdef ENABLE_DEC
    /* get the major number of the character device */
    if ((alloc_chrdev_region(&vdev->vdec_cdev_id, 0, 1,
                 CVI_VC_DRV_DECODER_DEV_NAME)) < 0) {
        err = -EBUSY;
        pr_err("could not allocate major number\n");
        return err;
    }
    vdev->s_vdec_major = MAJOR(vdev->vdec_cdev_id);
    vdev->p_vdec_cdev = vzalloc(sizeof(struct cdev));

    {
        dev_t subDevice;

        subDevice = MKDEV(vdev->s_vdec_major, 0);

        /* initialize the device structure and register the device with the kernel */
        cdev_init(vdev->p_vdec_cdev, &cvi_vc_drv_vdec_fops);
        vdev->p_vdec_cdev->owner = THIS_MODULE;

        if ((cdev_add(vdev->p_vdec_cdev, subDevice, 1)) < 0) {
            err = -EBUSY;
            pr_err("could not allocate chrdev\n");
            return err;
        }

        device_create(vdev->cvi_vc_class, NULL, subDevice, NULL, "%s",
                  CVI_VC_DRV_DECODER_DEV_NAME);
        for (i = 0; i < VDEC_MAX_CHN_NUM; i++) {
            init_waitqueue_head(&tVdecWaitQueue[i]);
            sema_init(&vdecSemArry[i], 1);
        }
        memset(vdec_chn_info, 0, sizeof(vdec_chn_info));
    }
#endif

    return err;
}

static int vc_drv_plat_probe(struct platform_device *pdev)
{
    int ret = 0;
    ret = jpeg_platform_init(pdev);
    ret = vpu_drv_platform_init(pdev);
    ret = cviVencDrvInit();
    ret = cviVdecDrvInit();

    venc_proc_init(&pdev->dev);
    h265e_proc_init(&pdev->dev);
    h264e_proc_init(&pdev->dev);
    jpege_proc_init(&pdev->dev);
    rc_proc_init(&pdev->dev);
    vdec_proc_init(&pdev->dev);
    codecinst_proc_init(&pdev->dev);

    return ret;
}

static int vc_drv_plat_remove(struct platform_device *pdev)
{
    int ret = 0;
    jpeg_platform_exit();
    vpu_drv_platform_exit();
    cviVencDrvDeinit();
    cviVdecDrvDeinit();

    venc_proc_deinit();
    h265e_proc_deinit();
    h264e_proc_deinit();
    jpege_proc_deinit();
    rc_proc_deinit();
    #ifdef ENABLE_DEC
    vdec_proc_deinit();
    #endif
    codecinst_proc_deinit();

    return ret;
}

#if defined(CONFIG_PM)
int cvi_vc_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
    vpu_drv_suspend(pdev, state);
    jpeg_drv_suspend(pdev, state);
    return 0;
}
int cvi_vc_drv_resume(struct platform_device *pdev)
{
    vpu_drv_resume(pdev);
    jpeg_drv_resume(pdev);
    return 0;
}
#endif



static const struct of_device_id vc_drv_match_table[] = {
    {.compatible = "sophgo,vc_drv"},
    {},
};

static struct platform_driver vc_plat_driver = {
    .driver = {
        .name = "sophgo,vc_drv",
        .of_match_table = vc_drv_match_table,
    },
    .probe      = vc_drv_plat_probe,
    .remove     = vc_drv_plat_remove,
    #if defined(CONFIG_PM)
    .suspend    = cvi_vc_drv_suspend,
    .resume     = cvi_vc_drv_resume,
    #endif
};

static int __init cvi_vc_drv_init(void)
{
    int ret = 0;
    struct cvi_vc_drv_device *vdev;

    vdev = vzalloc( sizeof(*vdev));
    if (!vdev) {
        return -ENOMEM;
    }

    memset(vdev, 0, sizeof(*vdev));

    ret = cvi_vc_drv_register_cdev(vdev);
    if (ret < 0) {
        pr_err("cvi_vc_drv_register_cdev fail\n");
        vfree(vdev);
        return -1;
    }

    pCviVcDrvDevice = vdev;

    ret = platform_driver_register(&vc_plat_driver);

    pr_info("cvi_vc_drv_init result = 0x%x\n", ret);

    return ret;
}

static void __exit cvi_vc_drv_exit(void)
{
    struct cvi_vc_drv_device *vdev = pCviVcDrvDevice;

    if (vdev->s_venc_major > 0) {
        dev_t subDevice;
        subDevice = MKDEV(vdev->s_venc_major, 0);
        cdev_del(vdev->p_venc_cdev);
        device_destroy(vdev->cvi_vc_class, subDevice);
        vfree(vdev->p_venc_cdev);
        unregister_chrdev_region(vdev->s_venc_major, 1);
        vdev->s_venc_major = 0;
    }

#ifdef ENABLE_DEC
    if (vdev->s_vdec_major > 0) {
        dev_t subDevice;

        subDevice = MKDEV(vdev->s_vdec_major, 0);
        cdev_del(vdev->p_vdec_cdev);
        device_destroy(vdev->cvi_vc_class, subDevice);
        vfree(vdev->p_vdec_cdev);
        unregister_chrdev_region(vdev->s_vdec_major, 1);
        vdev->s_vdec_major = 0;

    }
#endif
    class_destroy(vdev->cvi_vc_class);

    vfree(vdev);
    pCviVcDrvDevice = NULL;

    platform_driver_unregister(&vc_plat_driver);
}

MODULE_AUTHOR("vc sdk driver.");
MODULE_DESCRIPTION("vc sdk driver");
MODULE_LICENSE("GPL");

module_init(cvi_vc_drv_init);
module_exit(cvi_vc_drv_exit);
