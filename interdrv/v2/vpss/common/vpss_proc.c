#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include <linux/cvi_vip.h>

#include "vpss_debug.h"
#include "vpss_proc.h"
#include "vpss_common.h"
#include "scaler.h"
#include "vpss_core.h"
#include "vpss.h"

#define VPSS_PROC_NAME          "soph/vpss"

// for proc info
static int proc_vpss_mode;
static const char * const vb_source[] = {"CommonVB", "UserVB", "UserIon"};
static const char * const vpss_name[] = {"vpss_v0", "vpss_v1", "vpss_v2", "vpss_v3",
										"vpss_t0", "vpss_t1", "vpss_t2", "vpss_t3",
										"vpss_d0", "vpss_d1"};

/*************************************************************************
 *	VPSS proc functions
 *************************************************************************/
static void _pix_fmt_to_string(enum _PIXEL_FORMAT_E PixFmt, char *str, int len)
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
	case PIXEL_FORMAT_YUV_444:
		strncpy(str, "YUV_444", len);
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
	case PIXEL_FORMAT_FP16_C1:
		strncpy(str, "FP16_C1", len);
		break;
	case PIXEL_FORMAT_FP16_C3_PLANAR:
		strncpy(str, "FP16_C3_PLANAR", len);
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

int vpss_ctx_proc_show(struct seq_file *m, void *v)
{
	int i, j;
	char c[32];
	VPSS_MOD_PARAM_S stModParam;
	struct cvi_vpss_ctx **pVpssCtx = vpss_get_ctx();
	struct cvi_vpss_device *dev = (struct cvi_vpss_device *)m->private;
	signed int proc_amp[PROC_AMP_MAX];

	vpss_get_mod_param(&stModParam);

	// Module Param
	seq_printf(m, "\nModule: [VPSS], Build Time[%s]\n", UTS_VERSION);
	seq_puts(m, "\n-------------------------------MODULE PARAM-------------------------------\n");
	seq_printf(m, "%25s\n", "vpss_vb_source");
	seq_printf(m, "%25s\n", vb_source[stModParam.enVpssBufSource]);

	// VPSS GRP ATTR
	seq_puts(m, "\n-------------------------------VPSS GRP ATTR------------------------------\n");
	seq_printf(m, "%10s%10s%10s%20s%10s%10s\n", "GrpID", "MaxW", "MaxH", "PixFmt",
				"SrcFRate", "DstFRate");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pVpssCtx[i] && pVpssCtx[i]->isCreated) {
			memset(c, 0, sizeof(c));
			_pix_fmt_to_string(pVpssCtx[i]->stGrpAttr.enPixelFormat, c, sizeof(c));

			seq_printf(m, "%8s%2d%10d%10d%20s%10d%10d\n",
				"#",
				i,
				pVpssCtx[i]->stGrpAttr.u32MaxW,
				pVpssCtx[i]->stGrpAttr.u32MaxH,
				c,
				pVpssCtx[i]->stGrpAttr.stFrameRate.s32SrcFrameRate,
				pVpssCtx[i]->stGrpAttr.stFrameRate.s32DstFrameRate);
		}
	}

	// VPSS GRP AMP CTRL
	seq_puts(m, "\n-------------------------------VPSS GRP AMP CTRL------------------------------\n");
	seq_printf(m, "%10s%15s%13s%15s%8s\n", "GrpID", "Brightness", "Contrast", "Saturation",
				"Hue");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pVpssCtx[i] && pVpssCtx[i]->isCreated) {
			vpss_get_proc_amp(i, proc_amp);

			seq_printf(m, "%8s%2d%15d%13d%15d%8d\n",
				"#",
				i,
				proc_amp[PROC_AMP_BRIGHTNESS],
				proc_amp[PROC_AMP_CONTRAST],
				proc_amp[PROC_AMP_SATURATION],
				proc_amp[PROC_AMP_HUE]);

		}
	}

	//VPSS CHN ATTR
	seq_puts(m, "\n-------------------------------VPSS CHN ATTR------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s\n",
		"GrpID", "PhyChnID", "Enable", "MirrorEn", "FlipEn", "SrcFRate", "DstFRate");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s\n",
		"Depth", "Aspect", "videoX", "videoY", "videoW", "videoH", "BgColor");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pVpssCtx[i] && pVpssCtx[i]->isCreated) {
			for (j = 0; j < VPSS_MAX_CHN_NUM; ++j) {
				int32_t X, Y;
				uint32_t W, H;

				memset(c, 0, sizeof(c));
				if (pVpssCtx[i]->stChnCfgs[j].stChnAttr.stAspectRatio.enMode == ASPECT_RATIO_NONE)
					strncpy(c, "NONE", sizeof(c));
				else if (pVpssCtx[i]->stChnCfgs[j].stChnAttr.stAspectRatio.enMode == ASPECT_RATIO_AUTO)
					strncpy(c, "AUTO", sizeof(c));
				else if (pVpssCtx[i]->stChnCfgs[j].stChnAttr.stAspectRatio.enMode
						== ASPECT_RATIO_MANUAL)
					strncpy(c, "MANUAL", sizeof(c));
				else
					strncpy(c, "Invalid", sizeof(c));

				if (pVpssCtx[i]->stChnCfgs[j].stChnAttr.stAspectRatio.enMode == ASPECT_RATIO_MANUAL) {
					X = pVpssCtx[i]->stChnCfgs[j].stChnAttr.stAspectRatio.stVideoRect.s32X;
					Y = pVpssCtx[i]->stChnCfgs[j].stChnAttr.stAspectRatio.stVideoRect.s32Y;
					W = pVpssCtx[i]->stChnCfgs[j].stChnAttr.stAspectRatio.stVideoRect.u32Width;
					H = pVpssCtx[i]->stChnCfgs[j].stChnAttr.stAspectRatio.stVideoRect.u32Height;
				} else {
					X = Y = 0;
					W = H = 0;
				}

				seq_printf(m, "%8s%2d%8s%2d%10s%10s%10s%10d%10d\n%10d%10s%10d%10d%10d%10d%#10x\n",
					"#",
					i,
					"#",
					j,
					(pVpssCtx[i]->stChnCfgs[j].isEnabled) ? "Y" : "N",
					(pVpssCtx[i]->stChnCfgs[j].stChnAttr.bMirror) ? "Y" : "N",
					(pVpssCtx[i]->stChnCfgs[j].stChnAttr.bFlip) ? "Y" : "N",
					pVpssCtx[i]->stChnCfgs[j].stChnAttr.stFrameRate.s32SrcFrameRate,
					pVpssCtx[i]->stChnCfgs[j].stChnAttr.stFrameRate.s32DstFrameRate,
					pVpssCtx[i]->stChnCfgs[j].stChnAttr.u32Depth,
					c,
					X,
					Y,
					W,
					H,
					pVpssCtx[i]->stChnCfgs[j].stChnAttr.stAspectRatio.u32BgColor);
			}
		}
	}

	// VPSS GRP CROP INFO
	seq_puts(m, "\n-------------------------------VPSS GRP CROP INFO-------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s\n",
		"GrpID", "CropEn", "CoorType", "CoorX", "CoorY", "Width", "Height");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pVpssCtx[i] && pVpssCtx[i]->isCreated) {
			seq_printf(m, "%8s%2d%10s%10s%10d%10d%10d%10d\n",
				"#",
				i,
				(pVpssCtx[i]->stGrpCropInfo.bEnable) ? "Y" : "N",
				(pVpssCtx[i]->stGrpCropInfo.enCropCoordinate == VPSS_CROP_RATIO_COOR) ? "RAT" : "ABS",
				pVpssCtx[i]->stGrpCropInfo.stCropRect.s32X,
				pVpssCtx[i]->stGrpCropInfo.stCropRect.s32Y,
				pVpssCtx[i]->stGrpCropInfo.stCropRect.u32Width,
				pVpssCtx[i]->stGrpCropInfo.stCropRect.u32Height);
		}
	}

	// VPSS CHN CROP INFO
	seq_puts(m, "\n-------------------------------VPSS CHN CROP INFO-------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"GrpID", "ChnID", "CropEn", "CoorType", "CoorX", "CoorY", "Width", "Height");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pVpssCtx[i] && pVpssCtx[i]->isCreated) {
			for (j = 0; j < VPSS_MAX_CHN_NUM; ++j) {
				seq_printf(m, "%8s%2d%8s%2d%10s%10s%10d%10d%10d%10d\n",
					"#",
					i,
					"#",
					j,
					(pVpssCtx[i]->stChnCfgs[j].stCropInfo.bEnable) ? "Y" : "N",
					(pVpssCtx[i]->stChnCfgs[j].stCropInfo.enCropCoordinate
						== VPSS_CROP_RATIO_COOR) ? "RAT" : "ABS",
					pVpssCtx[i]->stChnCfgs[j].stCropInfo.stCropRect.s32X,
					pVpssCtx[i]->stChnCfgs[j].stCropInfo.stCropRect.s32Y,
					pVpssCtx[i]->stChnCfgs[j].stCropInfo.stCropRect.u32Width,
					pVpssCtx[i]->stChnCfgs[j].stCropInfo.stCropRect.u32Height);
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
		if (pVpssCtx[i] && pVpssCtx[i]->isCreated) {
			seq_printf(m, "%8s%2d%10d%10d%20d%10s%20d%20d%20d%20d\n",
				"#",
				i,
				pVpssCtx[i]->stGrpWorkStatus.u32RecvCnt,
				pVpssCtx[i]->stGrpWorkStatus.u32LostCnt,
				pVpssCtx[i]->stGrpWorkStatus.u32StartFailCnt,
				(pVpssCtx[i]->isStarted) ? "Y" : "N",
				pVpssCtx[i]->stGrpWorkStatus.u32CostTime,
				pVpssCtx[i]->stGrpWorkStatus.u32MaxCostTime,
				pVpssCtx[i]->stGrpWorkStatus.u32HwCostTime,
				pVpssCtx[i]->stGrpWorkStatus.u32HwMaxCostTime);
		}
	}

	// VPSS CHN OUTPUT RESOLUTION
	seq_puts(m, "\n-------------------------------VPSS CHN OUTPUT RESOLUTION-----------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%20s%10s%10s%10s\n",
		"GrpID", "ChnID", "Enable", "Width", "Height", "Pixfmt", "Videofmt", "SendOK", "FrameRate");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pVpssCtx[i] && pVpssCtx[i]->isCreated) {
			for (j = 0; j < VPSS_MAX_CHN_NUM; ++j) {
				memset(c, 0, sizeof(c));
				_pix_fmt_to_string(pVpssCtx[i]->stChnCfgs[j].stChnAttr.enPixelFormat, c, sizeof(c));

				seq_printf(m, "%8s%2d%8s%2d%10s%10d%10d%20s%10s%10d%10d\n",
					"#",
					i,
					"#",
					j,
					(pVpssCtx[i]->stChnCfgs[j].isEnabled) ? "Y" : "N",
					pVpssCtx[i]->stChnCfgs[j].stChnAttr.u32Width,
					pVpssCtx[i]->stChnCfgs[j].stChnAttr.u32Height,
					c,
					(pVpssCtx[i]->stChnCfgs[j].stChnAttr.enVideoFormat
						== VIDEO_FORMAT_LINEAR) ? "LINEAR" : "UNKNOWN",
					pVpssCtx[i]->stChnCfgs[j].stChnWorkStatus.u32SendOk,
					pVpssCtx[i]->stChnCfgs[j].stChnWorkStatus.u32RealFrameRate);
			}
		}
	}

	// VPSS CHN ROTATE INFO
	seq_puts(m, "\n-------------------------------VPSS CHN ROTATE INFO-----------------------\n");
	seq_printf(m, "%10s%10s%10s\n", "GrpID", "ChnID", "Rotate");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (pVpssCtx[i] && pVpssCtx[i]->isCreated) {
			for (j = 0; j < VPSS_MAX_CHN_NUM; ++j) {
				memset(c, 0, sizeof(c));
				if (pVpssCtx[i]->stChnCfgs[j].enRotation == ROTATION_0)
					strncpy(c, "0", sizeof(c));
				else if (pVpssCtx[i]->stChnCfgs[j].enRotation == ROTATION_90)
					strncpy(c, "90", sizeof(c));
				else if (pVpssCtx[i]->stChnCfgs[j].enRotation == ROTATION_180)
					strncpy(c, "180", sizeof(c));
				else if (pVpssCtx[i]->stChnCfgs[j].enRotation == ROTATION_270)
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
		if (pVpssCtx[i] && pVpssCtx[i]->isCreated) {
			for (j = 0; j < VPSS_MAX_CHN_NUM; ++j) {
				seq_printf(m, "%8s%2d%8s%2d%10s%10s%10d%10d\n%10d%10d%10d%20d\n",
					"#",
					i,
					"#",
					j,
					(pVpssCtx[i]->stChnCfgs[j].stLDCAttr.bEnable) ? "Y" : "N",
					(pVpssCtx[i]->stChnCfgs[j].stLDCAttr.stAttr.bAspect) ? "Y" : "N",
					pVpssCtx[i]->stChnCfgs[j].stLDCAttr.stAttr.s32XRatio,
					pVpssCtx[i]->stChnCfgs[j].stLDCAttr.stAttr.s32YRatio,
					pVpssCtx[i]->stChnCfgs[j].stLDCAttr.stAttr.s32XYRatio,
					pVpssCtx[i]->stChnCfgs[j].stLDCAttr.stAttr.s32CenterXOffset,
					pVpssCtx[i]->stChnCfgs[j].stLDCAttr.stAttr.s32CenterYOffset,
					pVpssCtx[i]->stChnCfgs[j].stLDCAttr.stAttr.s32DistortionRatio);
			}
		}
	}

	seq_puts(m, "\n-------------------------------VPSS HW STATUS-----------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s\n",
		"ID", "Dev", "Online", "Status", "StartCnt", "IntCnt", "DutyRatio");
	for (i = CVI_VPSS_V0; i < CVI_VPSS_MAX; ++i) {
		int state = atomic_read(&dev->vpss_cores[i].state);

		memset(c, 0, sizeof(c));
		if (state == CVI_VIP_IDLE)
			strncpy(c, "Idle", sizeof(c));
		else if (state == CVI_VIP_RUNNING)
			strncpy(c, "Running", sizeof(c));
		else if (state == CVI_VIP_END)
			strncpy(c, "End", sizeof(c));
		else if (state == CVI_VIP_ONLINE)
			strncpy(c, "Online", sizeof(c));

		seq_printf(m, "%8s%2d%10s%10s%10s%10d%10d%10d\n",
			"#",
			i,
			vpss_name[i],
			dev->vpss_cores[i].isOnline ? "Y" : "N",
			c,
			dev->vpss_cores[i].u32StartCnt,
			dev->vpss_cores[i].u32IntCnt,
			dev->vpss_cores[i].u32DutyRatio);
	}

	return 0;
}

static int vpss_proc_show(struct seq_file *m, void *v)
{
	// show driver status if vpss_mode == 1
	//if (proc_vpss_mode) {
	//}
	return vpss_ctx_proc_show(m, v);
}

static ssize_t vpss_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	char cProcInputdata[32] = {'\0'};

	if (user_buf == NULL || count >= sizeof(cProcInputdata)) {
		pr_err("Invalid input value\n");
		return -EINVAL;
	}

	if (copy_from_user(cProcInputdata, user_buf, count)) {
		pr_err("copy_from_user fail\n");
		return -EFAULT;
	}

	if (kstrtoint(cProcInputdata, 10, &proc_vpss_mode))
		proc_vpss_mode = 0;

	return count;
}

static int vpss_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, vpss_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops vpss_proc_fops = {
	.proc_open = vpss_proc_open,
	.proc_read = seq_read,
	.proc_write = vpss_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations vpss_proc_fops = {
	.owner = THIS_MODULE,
	.open = vpss_proc_open,
	.read = seq_read,
	.write = vpss_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int vpss_proc_init(struct cvi_vpss_device *dev)
{
	struct proc_dir_entry *entry;

	entry = proc_create_data(VPSS_PROC_NAME, 0644, NULL,
				 &vpss_proc_fops, dev);
	if (!entry) {
		CVI_TRACE_VPSS(CVI_DBG_ERR, "vpss proc creation failed\n");
		return -ENOMEM;
	}

	return 0;
}

int vpss_proc_remove(struct cvi_vpss_device *dev)
{
	remove_proc_entry(VPSS_PROC_NAME, NULL);
	return 0;
}
