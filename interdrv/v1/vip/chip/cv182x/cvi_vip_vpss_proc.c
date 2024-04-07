/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_vip_vpss_proc.c
 * Description: video pipeline scaling engine info
 */

#include "cvi_vip_vpss_proc.h"
#include <media/videobuf2-vmalloc.h>
#include "vip/vip_common.h"
#include "vip/scaler.h"
#include "cvi_vip_core.h"

static void *vpss_shared_mem;
/*************************************************************************
 *	VPSS proc functions
 *************************************************************************/
static void _pixFmt_to_String(enum _PIXEL_FORMAT_E PixFmt, char *str, int len)
{
	switch (PixFmt) {
	case PIXEL_FORMAT_RGB_888:
		strncpy(str, "RGB_888", len);
		break;
	case PIXEL_FORMAT_BGR_888:
		strncpy(str, "BGR_888", len);
		break;
	case PIXEL_FORMAT_RGB_888_PLANAR:
		strncpy(str, "RGB_888_PLANAR", len);
		break;
	case PIXEL_FORMAT_BGR_888_PLANAR:
		strncpy(str, "BGR_888_PLANAR", len);
		break;
	case PIXEL_FORMAT_ARGB_1555:
		strncpy(str, "ARGB_1555", len);
		break;
	case PIXEL_FORMAT_ARGB_4444:
		strncpy(str, "ARGB_4444", len);
		break;
	case PIXEL_FORMAT_ARGB_8888:
		strncpy(str, "ARGB_8888", len);
		break;
	case PIXEL_FORMAT_RGB_BAYER_8BPP:
		strncpy(str, "RGB_BAYER_8BPP", len);
		break;
	case PIXEL_FORMAT_RGB_BAYER_10BPP:
		strncpy(str, "RGB_BAYER_10BPP", len);
		break;
	case PIXEL_FORMAT_RGB_BAYER_12BPP:
		strncpy(str, "RGB_BAYER_12BPP", len);
		break;
	case PIXEL_FORMAT_RGB_BAYER_14BPP:
		strncpy(str, "RGB_BAYER_14BPP", len);
		break;
	case PIXEL_FORMAT_RGB_BAYER_16BPP:
		strncpy(str, "RGB_BAYER_16BPP", len);
		break;
	case PIXEL_FORMAT_YUV_PLANAR_422:
		strncpy(str, "YUV_PLANAR_422", len);
		break;
	case PIXEL_FORMAT_YUV_PLANAR_420:
		strncpy(str, "YUV_PLANAR_420", len);
		break;
	case PIXEL_FORMAT_YUV_PLANAR_444:
		strncpy(str, "YUV_PLANAR_444", len);
		break;
	case PIXEL_FORMAT_YUV_400:
		strncpy(str, "YUV_400", len);
		break;
	case PIXEL_FORMAT_HSV_888:
		strncpy(str, "HSV_888", len);
		break;
	case PIXEL_FORMAT_HSV_888_PLANAR:
		strncpy(str, "HSV_888_PLANAR", len);
		break;
	case PIXEL_FORMAT_NV12:
		strncpy(str, "NV12", len);
		break;
	case PIXEL_FORMAT_NV21:
		strncpy(str, "NV21", len);
		break;
	case PIXEL_FORMAT_NV16:
		strncpy(str, "NV16", len);
		break;
	case PIXEL_FORMAT_NV61:
		strncpy(str, "NV61", len);
		break;
	case PIXEL_FORMAT_YUYV:
		strncpy(str, "YUYV", len);
		break;
	case PIXEL_FORMAT_UYVY:
		strncpy(str, "UYVY", len);
		break;
	case PIXEL_FORMAT_YVYU:
		strncpy(str, "YVYU", len);
		break;
	case PIXEL_FORMAT_VYUY:
		strncpy(str, "VYUY", len);
		break;
	case PIXEL_FORMAT_FP32_C1:
		strncpy(str, "FP32_C1", len);
		break;
	case PIXEL_FORMAT_FP32_C3_PLANAR:
		strncpy(str, "FP32_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_INT32_C1:
		strncpy(str, "INT32_C1", len);
		break;
	case PIXEL_FORMAT_INT32_C3_PLANAR:
		strncpy(str, "INT32_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_UINT32_C1:
		strncpy(str, "UINT32_C1", len);
		break;
	case PIXEL_FORMAT_UINT32_C3_PLANAR:
		strncpy(str, "UINT32_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_BF16_C1:
		strncpy(str, "BF16_C1", len);
		break;
	case PIXEL_FORMAT_BF16_C3_PLANAR:
		strncpy(str, "BF16_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_INT16_C1:
		strncpy(str, "INT16_C1", len);
		break;
	case PIXEL_FORMAT_INT16_C3_PLANAR:
		strncpy(str, "INT16_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_UINT16_C1:
		strncpy(str, "UINT16_C1", len);
		break;
	case PIXEL_FORMAT_UINT16_C3_PLANAR:
		strncpy(str, "UINT16_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_INT8_C1:
		strncpy(str, "INT8_C1", len);
		break;
	case PIXEL_FORMAT_INT8_C3_PLANAR:
		strncpy(str, "INT8_C3_PLANAR", len);
		break;
	case PIXEL_FORMAT_UINT8_C1:
		strncpy(str, "UINT8_C1", len);
		break;
	case PIXEL_FORMAT_UINT8_C3_PLANAR:
		strncpy(str, "UINT8_C3_PLANAR", len);
		break;
	default:
		strncpy(str, "Unknown Fmt", len);
		break;
	}
}

int vpss_proc_show(struct seq_file *m, void *v)
{
	struct cvi_vpss_proc_ctx *pvpssCtx = NULL;
	int i, j;
	char c[32];
	struct cvi_vip_dev *bdev = m->private;
	bool isSingleMode = bdev->img_vdev[CVI_VIP_IMG_D].sc_bounding ==
							CVI_VIP_IMG_2_SC_NONE ? true : false;

	pvpssCtx = (struct cvi_vpss_proc_ctx *)(vpss_shared_mem);
	if (!pvpssCtx) {
		seq_puts(m, "vpss shm = NULL\n");
		return -1;
	}

	// Module Param
	seq_printf(m, "\nModule: [VPSS], Build Time[%s]\n", UTS_VERSION);
	seq_puts(m, "\n-------------------------------MODULE PARAM-------------------------------\n");
	seq_printf(m, "%25s%25s\n", "vpss_vb_source", "vpss_split_node_num");
	seq_printf(m, "%18d%25d\n", 0, 1);
	seq_puts(m, "\n-------------------------------VPSS MODE----------------------------------\n");
	seq_printf(m, "%25s%15s%15s\n", "vpss_mode", "dev0", "dev1");
	seq_printf(m, "%25s%15s%15s\n", isSingleMode ? "single" : "dual", isSingleMode ? "N" :
		(bdev->img_vdev[CVI_VIP_IMG_D].is_online_from_isp ? "input_isp" : "input_mem"),
		bdev->img_vdev[CVI_VIP_IMG_V].is_online_from_isp ? "input_isp" : "input_mem");

	// VPSS GRP ATTR
	seq_puts(m, "\n-------------------------------VPSS GRP ATTR------------------------------\n");
	seq_printf(m, "%10s%10s%10s%20s%10s%10s%5s\n", "GrpID", "MaxW", "MaxH", "PixFmt",
				"SrcFRate", "DstFRate", "dev");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pvpssCtx[i].isCreated) {
			memset(c, 0, sizeof(c));
			_pixFmt_to_String(pvpssCtx[i].stGrpAttr.enPixelFormat, c, sizeof(c));

			seq_printf(m, "%8s%2d%10d%10d%20s%10d%10d%5d\n",
				"#",
				i,
				pvpssCtx[i].stGrpAttr.u32MaxW,
				pvpssCtx[i].stGrpAttr.u32MaxH,
				c,
				pvpssCtx[i].stGrpAttr.stFrameRate.s32SrcFrameRate,
				pvpssCtx[i].stGrpAttr.stFrameRate.s32DstFrameRate,
				pvpssCtx[i].stGrpAttr.u8VpssDev);
		}
	}

	//VPSS CHN ATTR
	seq_puts(m, "\n-------------------------------VPSS CHN ATTR------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s\n",
		"GrpID", "PhyChnID", "Enable", "MirrorEn", "FlipEn", "SrcFRate", "DstFRate");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s\n",
		"Depth", "Aspect", "videoX", "videoY", "videoW", "videoH", "BgColor");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pvpssCtx[i].isCreated) {
			for (j = 0; j < pvpssCtx[i].chnNum; ++j) {
				int32_t X, Y;
				uint32_t W, H;

				memset(c, 0, sizeof(c));
				if (pvpssCtx[i].stChnCfgs[j].stChnAttr.stAspectRatio.enMode == ASPECT_RATIO_NONE)
					strncpy(c, "NONE", sizeof(c));
				else if (pvpssCtx[i].stChnCfgs[j].stChnAttr.stAspectRatio.enMode == ASPECT_RATIO_AUTO)
					strncpy(c, "AUTO", sizeof(c));
				else if (pvpssCtx[i].stChnCfgs[j].stChnAttr.stAspectRatio.enMode == ASPECT_RATIO_MANUAL)
					strncpy(c, "MANUAL", sizeof(c));
				else
					strncpy(c, "Invalid", sizeof(c));

				if (pvpssCtx[i].stChnCfgs[j].stChnAttr.stAspectRatio.enMode == ASPECT_RATIO_MANUAL) {
					X = pvpssCtx[i].stChnCfgs[j].stChnAttr.stAspectRatio.stVideoRect.s32X;
					Y = pvpssCtx[i].stChnCfgs[j].stChnAttr.stAspectRatio.stVideoRect.s32Y;
					W = pvpssCtx[i].stChnCfgs[j].stChnAttr.stAspectRatio.stVideoRect.u32Width;
					H = pvpssCtx[i].stChnCfgs[j].stChnAttr.stAspectRatio.stVideoRect.u32Height;
				} else {
					X = Y = 0;
					W = H = 0;
				}

				seq_printf(m, "%8s%2d%8s%2d%10s%10s%10s%10d%10d\n%10d%10s%10d%10d%10d%10d%#10x\n",
					"#",
					i,
					"#",
					j,
					(pvpssCtx[i].stChnCfgs[j].isEnabled) ? "Y" : "N",
					(pvpssCtx[i].stChnCfgs[j].stChnAttr.bMirror) ? "Y" : "N",
					(pvpssCtx[i].stChnCfgs[j].stChnAttr.bFlip) ? "Y" : "N",
					pvpssCtx[i].stChnCfgs[j].stChnAttr.stFrameRate.s32SrcFrameRate,
					pvpssCtx[i].stChnCfgs[j].stChnAttr.stFrameRate.s32DstFrameRate,
					pvpssCtx[i].stChnCfgs[j].stChnAttr.u32Depth,
					c,
					X,
					Y,
					W,
					H,
					pvpssCtx[i].stChnCfgs[j].stChnAttr.stAspectRatio.u32BgColor);
			}
		}
	}

	// VPSS GRP CROP INFO
	seq_puts(m, "\n-------------------------------VPSS GRP CROP INFO-------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s\n",
		"GrpID", "CropEn", "CoorType", "CoorX", "CoorY", "Width", "Height");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pvpssCtx[i].isCreated) {
			seq_printf(m, "%8s%2d%10s%10s%10d%10d%10d%10d\n",
				"#",
				i,
				(pvpssCtx[i].stGrpCropInfo.bEnable) ? "Y" : "N",
				(pvpssCtx[i].stGrpCropInfo.enCropCoordinate == VPSS_CROP_RATIO_COOR) ? "RAT" : "ABS",
				pvpssCtx[i].stGrpCropInfo.stCropRect.s32X,
				pvpssCtx[i].stGrpCropInfo.stCropRect.s32Y,
				pvpssCtx[i].stGrpCropInfo.stCropRect.u32Width,
				pvpssCtx[i].stGrpCropInfo.stCropRect.u32Height);
		}
	}

	// VPSS CHN CROP INFO
	seq_puts(m, "\n-------------------------------VPSS CHN CROP INFO-------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"GrpID", "ChnID", "CropEn", "CoorType", "CoorX", "CoorY", "Width", "Height");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pvpssCtx[i].isCreated) {
			for (j = 0; j < pvpssCtx[i].chnNum; ++j) {
				seq_printf(m, "%8s%2d%8s%2d%10s%10s%10d%10d%10d%10d\n",
					"#",
					i,
					"#",
					j,
					(pvpssCtx[i].stChnCfgs[j].stCropInfo.bEnable) ? "Y" : "N",
					(pvpssCtx[i].stChnCfgs[j].stCropInfo.enCropCoordinate
						== VPSS_CROP_RATIO_COOR) ? "RAT" : "ABS",
					pvpssCtx[i].stChnCfgs[j].stCropInfo.stCropRect.s32X,
					pvpssCtx[i].stChnCfgs[j].stCropInfo.stCropRect.s32Y,
					pvpssCtx[i].stChnCfgs[j].stCropInfo.stCropRect.u32Width,
					pvpssCtx[i].stChnCfgs[j].stCropInfo.stCropRect.u32Height);
			}
		}
	}

	// VPSS GRP WORK STATUS
	seq_puts(m, "\n-------------------------------VPSS GRP WORK STATUS-----------------------\n");
	seq_printf(m, "%10s%10s%10s%20s%10s%20s%20s%20s%20s\n",
		"GrpID", "RecvCnt", "LostCnt", "StartFailCnt", "bStart",
		"CostTime(us)", "MaxCostTime(us)",
		"HwCostTime(us)", "HwMaxCostTime(us)");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pvpssCtx[i].isCreated) {
			seq_printf(m, "%8s%2d%10d%10d%20d%10s%20d%20d%20d%20d\n",
				"#",
				i,
				pvpssCtx[i].stGrpWorkStatus.u32RecvCnt,
				pvpssCtx[i].stGrpWorkStatus.u32LostCnt,
				pvpssCtx[i].stGrpWorkStatus.u32StartFailCnt,
				(pvpssCtx[i].isStarted) ? "Y" : "N",
				pvpssCtx[i].stGrpWorkStatus.u32CostTime,
				pvpssCtx[i].stGrpWorkStatus.u32MaxCostTime,
				pvpssCtx[i].stGrpWorkStatus.u32HwCostTime,
				pvpssCtx[i].stGrpWorkStatus.u32HwMaxCostTime);
		}
	}

	// VPSS CHN LOW DELAY INFO
	seq_puts(m, "\n-------------------------------VPSS CHN LOW DELAY INFO-----------------\n");
	seq_printf(m, "%10s%10s%10s%10s%20s%20s\n",
		"GrpID", "ChnID", "Enable", "LineCnt", "CostTime(us)", "MaxCostTime(us)");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pvpssCtx[i].isCreated) {
			for (j = 0; j < pvpssCtx[i].chnNum; ++j) {
				seq_printf(m, "%8s%2d%8s%2d%10s%10d%20d%20d\n",
					"#",
					i,
					"#",
					j,
					(pvpssCtx[i].stChnCfgs[j].stLowDelayInfo.bEnable) ? "Y" : "N",
					pvpssCtx[i].stChnCfgs[j].stLowDelayInfo.u32LineCnt,
					pvpssCtx[i].stChnCfgs[j].u32LowDelayTime,
					pvpssCtx[i].stChnCfgs[j].u32LowDelayMaxTime);
			}
		}
	}

	// VPSS CHN OUTPUT RESOLUTION
	seq_puts(m, "\n-------------------------------VPSS CHN OUTPUT RESOLUTION-----------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%20s%10s%10s%10s\n",
		"GrpID", "ChnID", "Enable", "Width", "Height", "Pixfmt", "Videofmt", "SendOK", "FrameRate");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pvpssCtx[i].isCreated) {
			for (j = 0; j < pvpssCtx[i].chnNum; ++j) {
				memset(c, 0, sizeof(c));
				_pixFmt_to_String(pvpssCtx[i].stChnCfgs[j].stChnAttr.enPixelFormat, c, sizeof(c));

				seq_printf(m, "%8s%2d%8s%2d%10s%10d%10d%20s%10s%10d%10d\n",
					"#",
					i,
					"#",
					j,
					(pvpssCtx[i].stChnCfgs[j].isEnabled) ? "Y" : "N",
					pvpssCtx[i].stChnCfgs[j].stChnAttr.u32Width,
					pvpssCtx[i].stChnCfgs[j].stChnAttr.u32Height,
					c,
					(pvpssCtx[i].stChnCfgs[j].stChnAttr.enVideoFormat
						== VIDEO_FORMAT_LINEAR) ? "LINEAR" : "UNKNOWN",
					pvpssCtx[i].stChnCfgs[j].u32SendOk,
					pvpssCtx[i].stChnCfgs[j].u32RealFrameRate);
			}
		}
	}

	// VPSS CHN ROTATE INFO
	seq_puts(m, "\n-------------------------------VPSS CHN ROTATE INFO-----------------------\n");
	seq_printf(m, "%10s%10s%10s\n", "GrpID", "ChnID", "Rotate");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pvpssCtx[i].isCreated) {
			for (j = 0; j < pvpssCtx[i].chnNum; ++j) {
				memset(c, 0, sizeof(c));
				if (pvpssCtx[i].stChnCfgs[j].enRotation == ROTATION_0)
					strncpy(c, "0", sizeof(c));
				else if (pvpssCtx[i].stChnCfgs[j].enRotation == ROTATION_90)
					strncpy(c, "90", sizeof(c));
				else if (pvpssCtx[i].stChnCfgs[j].enRotation == ROTATION_180)
					strncpy(c, "180", sizeof(c));
				else if (pvpssCtx[i].stChnCfgs[j].enRotation == ROTATION_270)
					strncpy(c, "270", sizeof(c));
				else
					strncpy(c, "Invalid", sizeof(c));

				seq_printf(m, "%8s%2d%8s%2d%10s\n", "#", i, "#", j, c);
			}
		}
	}

	// VPSS CHN LDC INFO
	seq_puts(m, "\n-------------------------------VPSS CHN LDC INFO-----------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s\n", "GrpID", "ChnID", "Enable", "Aspect", "XRatio", "YRatio");
	seq_printf(m, "%10s%10s%10s%20s\n", "XYRatio", "XOffset", "YOffset", "DistortionRatio");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pvpssCtx[i].isCreated) {
			for (j = 0; j < pvpssCtx[i].chnNum; ++j) {
				seq_printf(m, "%8s%2d%8s%2d%10s%10s%10d%10d\n%10d%10d%10d%20d\n",
					"#",
					i,
					"#",
					j,
					(pvpssCtx[i].stChnCfgs[j].stLDCAttr.bEnable) ? "Y" : "N",
					(pvpssCtx[i].stChnCfgs[j].stLDCAttr.stAttr.bAspect) ? "Y" : "N",
					pvpssCtx[i].stChnCfgs[j].stLDCAttr.stAttr.s32XRatio,
					pvpssCtx[i].stChnCfgs[j].stLDCAttr.stAttr.s32YRatio,
					pvpssCtx[i].stChnCfgs[j].stLDCAttr.stAttr.s32XYRatio,
					pvpssCtx[i].stChnCfgs[j].stLDCAttr.stAttr.s32CenterXOffset,
					pvpssCtx[i].stChnCfgs[j].stLDCAttr.stAttr.s32CenterYOffset,
					pvpssCtx[i].stChnCfgs[j].stLDCAttr.stAttr.s32DistortionRatio);
			}
		}
	}

	//VPSS driver status
	seq_puts(m, "\n------------------------------DRV WORK STATUS------------------------------\n");
	seq_printf(m, "%14s%20s%20s%20s%20s\n", "dev", "IspTrigCnt0", "IspTrigCnt1",
			"IspTrigFailCnt0", "IspTrigFailCnt1");
	seq_printf(m, "%14s%20s%20s%20s%20s\n",
		"UserTrigCnt", "UserTrigFailCnt", "ofl_IrqCnt", "ol_IrqCnt0", "ol_IrqCnt1");
	for (i = 0; i < CVI_VIP_IMG_MAX; ++i) {
		seq_printf(m, "%12s%2d%20d%20d%20d%20d\n%14d%20d%20d%20d%20d\n",
			"#",
			i,
			bdev->img_vdev[i].isp_trig_cnt[0], bdev->img_vdev[i].isp_trig_cnt[1],
			bdev->img_vdev[i].isp_trig_fail_cnt[0], bdev->img_vdev[i].isp_trig_fail_cnt[1],
			bdev->img_vdev[i].user_trig_cnt,
			bdev->img_vdev[i].user_trig_fail_cnt, bdev->img_vdev[i].irq_cnt,
			bdev->img_vdev[i].ol_irq_cnt[0], bdev->img_vdev[i].ol_irq_cnt[1]);
	}

	return 0;
}

int vpss_proc_init(void *shm)
{
	int rc = 0;

	vpss_shared_mem = shm;
	return rc;
}

int vpss_proc_remove(void)
{
	vpss_shared_mem = NULL;

	return 0;
}
