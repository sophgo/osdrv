#include <proc/vo_proc.h>
#include <linux/version.h>
#include <linux/cvi_vip.h>
#include "cvi_vo_ctx.h"

#define VO_PRC_NAME	"soph/vo"

/*************************************************************************
 *	VO proc functions
 *************************************************************************/
static void _intf_type_to_string(uint32_t intfType, char *str, int len)
{
	switch (intfType) {
	case VO_INTF_CVBS:
		strncpy(str, "CVBS", len);
		break;
	case VO_INTF_YPBPR:
		strncpy(str, "YPBPR", len);
		break;
	case VO_INTF_VGA:
		strncpy(str, "VGA", len);
		break;
	case VO_INTF_BT656:
		strncpy(str, "BT656", len);
		break;
	case VO_INTF_BT1120:
		strncpy(str, "BT1120", len);
		break;
	case VO_INTF_LCD:
		strncpy(str, "LCD", len);
		break;
	case VO_INTF_LCD_18BIT:
		strncpy(str, "LCD_18BIT", len);
		break;
	case VO_INTF_LCD_24BIT:
		strncpy(str, "LCD_24BIT", len);
		break;
	case VO_INTF_LCD_30BIT:
		strncpy(str, "LCD_30BIT", len);
		break;
	case VO_INTF_MIPI:
		strncpy(str, "MIPI", len);
		break;
	case VO_INTF_MIPI_SLAVE:
		strncpy(str, "MIPI_SLAVE", len);
		break;
	case VO_INTF_HDMI:
		strncpy(str, "HDMI", len);
		break;
	case VO_INTF_I80:
		strncpy(str, "I80", len);
		break;
	default:
		strncpy(str, "Unknown Type", len);
		break;
	}
}

static void _intf_sync_to_string(enum _VO_INTF_SYNC_E intfSync, char *str, int len)
{
	switch (intfSync) {
	case VO_OUTPUT_PAL:
		strncpy(str, "PAL", len);
		break;
	case VO_OUTPUT_NTSC:
		strncpy(str, "NTSC", len);
		break;
	case VO_OUTPUT_1080P24:
		strncpy(str, "1080P@24", len);
		break;
	case VO_OUTPUT_1080P25:
		strncpy(str, "1080P@25", len);
		break;
	case VO_OUTPUT_1080P30:
		strncpy(str, "1080P@30", len);
		break;
	case VO_OUTPUT_720P50:
		strncpy(str, "720P@50", len);
		break;
	case VO_OUTPUT_720P60:
		strncpy(str, "720P@60", len);
		break;
	case VO_OUTPUT_1080P50:
		strncpy(str, "1080P@50", len);
		break;
	case VO_OUTPUT_1080P60:
		strncpy(str, "1080P@60", len);
		break;
	case VO_OUTPUT_576P50:
		strncpy(str, "576P@50", len);
		break;
	case VO_OUTPUT_480P60:
		strncpy(str, "480P@60", len);
		break;
	case VO_OUTPUT_800x600_60:
		strncpy(str, "800x600@60", len);
		break;
	case VO_OUTPUT_1024x768_60:
		strncpy(str, "1024x768@60", len);
		break;
	case VO_OUTPUT_1280x1024_60:
		strncpy(str, "1280x1024@60", len);
		break;
	case VO_OUTPUT_1366x768_60:
		strncpy(str, "1366x768@60", len);
		break;
	case VO_OUTPUT_1440x900_60:
		strncpy(str, "1440x900@60", len);
		break;
	case VO_OUTPUT_1280x800_60:
		strncpy(str, "1280x800@60", len);
		break;
	case VO_OUTPUT_1600x1200_60:
		strncpy(str, "1600x1200@60", len);
		break;
	case VO_OUTPUT_1680x1050_60:
		strncpy(str, "1680x1050@60", len);
		break;
	case VO_OUTPUT_1920x1200_60:
		strncpy(str, "1920x1200@60", len);
		break;
	case VO_OUTPUT_640x480_60:
		strncpy(str, "640x480@60", len);
		break;
	case VO_OUTPUT_720x1280_60:
		strncpy(str, "720x1280@60", len);
		break;
	case VO_OUTPUT_1080x1920_60:
		strncpy(str, "1080x1920@60", len);
		break;
	case VO_OUTPUT_480x800_60:
		strncpy(str, "480x800@60", len);
		break;
	case VO_OUTPUT_1440P60:
		strncpy(str, "1440P@60", len);
		break;
	case VO_OUTPUT_USER:
		strncpy(str, "User timing", len);
		break;
	default:
		strncpy(str, "Unknown Timing", len);
		break;
	}
}

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

static void _wbcSrc_to_String(VO_WBC_SRC_TYPE_E enSrcType, char *str, int len)
{
	switch (enSrcType) {
	case VO_WBC_SRC_DEV:
		strncpy(str, "DEV", len);
		break;
	case VO_WBC_SRC_VIDEO:
		strncpy(str, "VIDEO", len);
		break;
	default:
		strncpy(str, "Unknown", len);
		break;
	}
}

static void _wbcMode_to_String(VO_WBC_MODE_E enWbcMode, char *str, int len)
{
	switch (enWbcMode) {
	case VO_WBC_MODE_NORM:
		strncpy(str, "NORM", len);
		break;
	case VO_WBC_MODE_DROP_REPEAT:
		strncpy(str, "REPEAT", len);
		break;
	case VO_WBC_MODE_PROGRESSIVE_TO_INTERLACED:
		strncpy(str, "INTERLACED", len);
		break;
	default:
		strncpy(str, "Unknown", len);
		break;
	}
}

static void _chnAspectRatio_to_String(ASPECT_RATIO_E enMode, char *str, int len)
{
	switch (enMode) {
	case ASPECT_RATIO_NONE:
		strncpy(str, "NONE", len);
		break;
	case ASPECT_RATIO_AUTO:
		strncpy(str, "AUTO", len);
		break;
	case ASPECT_RATIO_MANUAL:
		strncpy(str, "MANUAL", len);
		break;
	default:
		strncpy(str, "Unknown", len);
		break;
	}
}

static void _chnZoomType_to_String(VO_CHN_ZOOM_TYPE enZoomType, char *str, int len)
{
	switch (enZoomType) {
	case VO_CHN_ZOOM_IN_RECT:
		strncpy(str, "RECT", len);
		break;
	case VO_CHN_ZOOM_IN_RATIO:
		strncpy(str, "RATIO", len);
		break;
	default:
		strncpy(str, "Unknown", len);
		break;
	}
}

static void _chnMirror_to_String(VO_CHN_MIRROR_TYPE enMirror, char *str, int len)
{
	switch (enMirror) {
	case VO_CHN_MIRROR_NONE:
		strncpy(str, "NONE", len);
		break;
	case VO_CHN_MIRROR_HOR:
		strncpy(str, "HOR", len);
		break;
	case VO_CHN_MIRROR_VER:
		strncpy(str, "VER", len);
		break;
	case VO_CHN_MIRROR_BOTH:
		strncpy(str, "BOTH", len);
		break;
	default:
		strncpy(str, "Unknown", len);
		break;
	}
}

static int _vo_proc_show(struct seq_file *m, void *v)
{
	int i, j, cnt;
	char c[32], d[32];
	struct cvi_vo_ctx *pvoCtx = (struct cvi_vo_ctx *)m->private;

#if 0//TODO: UTS_VERSION
	seq_printf(m, "\nModule: [VO], Build Time[%s]\n", UTS_VERSION);
#endif
	// Device Config
	seq_puts(m, "\n-------------------------------DEVICE CONFIG------------------------------\n");
	seq_printf(m, "%10s%10s%20s%20s%10s%10s\n", "DevID", "DevEn", "IntfType", "IntfSync", "BkClr", "DevFrt");
	for (i = 0; i < VO_MAX_DEV_NUM; ++i) {
		memset(c, 0, sizeof(c));
		_intf_type_to_string(pvoCtx->astDevCtx[i].stPubAttr.enIntfType, c, sizeof(c));

		memset(d, 0, sizeof(d));
		_intf_sync_to_string(pvoCtx->astDevCtx[i].stPubAttr.enIntfSync, d, sizeof(d));
		seq_printf(m, "%8s%2d%10s%20s%20s%10X%10d\n",
				"#",
				i,
				(pvoCtx->astDevCtx[i].is_dev_enable) ? "Y" : "N",
				c,
				d,
				pvoCtx->astDevCtx[i].stPubAttr.u32BgColor,
				pvoCtx->astDevCtx[i].stPubAttr.stSyncInfo.u16FrameRate);
	}

	// video layer status 1
	seq_puts(m, "\n-------------------------------VIDEO LAYER STATUS 1-----------------------\n");
	seq_printf(m, "%10s%10s%20s%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "VideoEn", "PixFmt", "ImgW", "ImgH", "DispX", "DispY", "DispW", "DispH");
	for (i = 0; i < VO_MAX_LAYER_NUM; ++i) {
		memset(c, 0, sizeof(c));
		_pix_fmt_to_string(pvoCtx->astLayerCtx[i].stLayerAttr.enPixFormat, c, sizeof(c));

		seq_printf(m, "%8s%2d%10s%20s%10d%10d%10d%10d%10d%10d\n",
				"#",
				i,
				(pvoCtx->astLayerCtx[i].is_layer_enable) ? "Y" : "N",
				c,
				pvoCtx->astLayerCtx[i].stLayerAttr.stImageSize.u32Width,
				pvoCtx->astLayerCtx[i].stLayerAttr.stImageSize.u32Height,
				pvoCtx->astLayerCtx[i].stLayerAttr.stDispRect.s32X,
				pvoCtx->astLayerCtx[i].stLayerAttr.stDispRect.s32Y,
				pvoCtx->astLayerCtx[i].stLayerAttr.stDispRect.u32Width,
				pvoCtx->astLayerCtx[i].stLayerAttr.stDispRect.u32Height);
	}

	// video layer status 2
	seq_puts(m, "\n-------------------------------VIDEO LAYER STATUS 2 (continue)------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%15s%20s%20s\n",
		"LayerId", "BindDevId", "EnChNum", "Luma", "Cont", "Hue", "Satu", "Toleration", "DispalyPts", "PreDonePts");
	for (i = 0; i < VO_MAX_LAYER_NUM; ++i) {

		cnt = 0;
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (pvoCtx->astLayerCtx[i].astChnCtx[j].is_chn_enable)
				cnt++;
		}

		seq_printf(m, "%8s%2d%10d%10d%10d%10d%10d%10d%15d%20lld%20lld\n",
				"#",
				i,
				pvoCtx->astLayerCtx[i].s32BindDevId,
				cnt,
				pvoCtx->astLayerCtx[i].proc_amp[PROC_AMP_BRIGHTNESS],
				pvoCtx->astLayerCtx[i].proc_amp[PROC_AMP_CONTRAST],
				pvoCtx->astLayerCtx[i].proc_amp[PROC_AMP_HUE],
				pvoCtx->astLayerCtx[i].proc_amp[PROC_AMP_SATURATION],
				pvoCtx->astLayerCtx[i].u32Toleration,
				pvoCtx->astLayerCtx[i].u64DisplayPts,
				pvoCtx->astLayerCtx[i].u64PreDonePts);
	}

	// video layer status 3
	seq_puts(m, "\n-------------------------------VIDEO LAYER STATUS 3 (continue)------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "BufLen", "Depth", "SrcFrt", "RealFrt", "BwFail", "OsdBwFail");
	for (i = 0; i < VO_MAX_LAYER_NUM; ++i) {

		cnt = 0;
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (pvoCtx->astLayerCtx[i].astChnCtx[j].is_chn_enable)
				cnt++;
		}

		seq_printf(m, "%8s%2d%10d%10d%10d%10d%10d%10d\n",
				"#",
				i,
				pvoCtx->astLayerCtx[i].u32DisBufLen,
				pvoCtx->astLayerCtx[i].stLayerAttr.u32Depth,
				pvoCtx->astLayerCtx[i].u32LayerSrcFrameRate,
				pvoCtx->astLayerCtx[i].u32LayerFrameRate,
				pvoCtx->astLayerCtx[i].u32BwFail,
				pvoCtx->astLayerCtx[i].u32OsdBwFail);
	}

	// gragphic layer status 1
	seq_puts(m, "\n-------------------------------GRAPHIC LAYER STATUS 1-----------------------\n");
	seq_printf(m, "%10s%10s%10s\n",
		"LayerId", "BindDevId", "Prio");
	for (i = VO_MAX_LAYER_NUM; i < VO_MAX_LAYER_NUM + VO_MAX_OVERLAY_NUM; ++i) {

		seq_printf(m, "%8s%2d%10d%10d\n",
				"#",
				i,
				pvoCtx->astOverlayCtx[i - VO_MAX_LAYER_NUM].s32BindDevId,
				pvoCtx->astOverlayCtx[i - VO_MAX_LAYER_NUM].u32Priority);
	}

	// chn basic info
	seq_puts(m, "\n-------------------------------CHN BASIC INFO 1---------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "ChnId", "ChnEn", "Prio", "SrcW", "SrcH", "ChnX", "ChnY", "ChnW", "ChnH", "RotAngle", "Thrshd", "Depth");
	for (i = 0; i < VO_MAX_LAYER_NUM; ++i) {
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (!pvoCtx->astLayerCtx[i].astChnCtx[j].is_chn_enable)
				continue;
			memset(c, 0, sizeof(c));
			if (pvoCtx->astLayerCtx[i].astChnCtx[j].enRotation == ROTATION_0)
				strncpy(c, "0", sizeof(c));
			else if (pvoCtx->astLayerCtx[i].astChnCtx[j].enRotation == ROTATION_90)
				strncpy(c, "90", sizeof(c));
			else if (pvoCtx->astLayerCtx[i].astChnCtx[j].enRotation == ROTATION_180)
				strncpy(c, "180", sizeof(c));
			else if (pvoCtx->astLayerCtx[i].astChnCtx[j].enRotation == ROTATION_270)
				strncpy(c, "270", sizeof(c));
			else
				strncpy(c, "Invalid", sizeof(c));

			seq_printf(m, "%8s%2d%8s%2d%10s%10d%10d%10d%10d%10d%10d%10d%10s%10d%10d\n",
				"#",
				i,
				"#",
				j,
				(pvoCtx->astLayerCtx[i].astChnCtx[j].is_chn_enable) ? "Y" : "N",
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnAttr.u32Priority,
				pvoCtx->astLayerCtx[i].astChnCtx[j].u32SrcWidth,
				pvoCtx->astLayerCtx[i].astChnCtx[j].u32SrcHeight,
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnAttr.stRect.s32X,
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnAttr.stRect.s32Y,
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnAttr.stRect.u32Width,
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnAttr.stRect.u32Height,
				c,
				pvoCtx->astLayerCtx[i].astChnCtx[j].u32Threshold,
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnAttr.u32Depth);
		}
	}

	seq_puts(m, "\n-------------------------------CHN ZOOM INFO-----------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "ChnId", "ZoomType", "RectX", "RectY", "RectW", "RectH", "Xratio", "Yratio", "WRatio", "HRatio");
	for (i = 0; i < VO_MAX_LAYER_NUM; ++i) {
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (!pvoCtx->astLayerCtx[i].astChnCtx[j].is_chn_enable)
				continue;
			memset(c, 0, sizeof(c));
			_chnZoomType_to_String(pvoCtx->astLayerCtx[i].astChnCtx[j].stChnZoomAttr.enZoomType, c, sizeof(c));

			seq_printf(m, "%8s%2d%8s%2d%10s%10d%10d%10d%10d%10d%10d%10d%10d\n",
				"#",
				i,
				"#",
				j,
				c,
				(!pvoCtx->astLayerCtx[i].astChnCtx[j].stChnZoomAttr.enZoomType) ?
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnZoomAttr.stRect.s32X : 0,
				(!pvoCtx->astLayerCtx[i].astChnCtx[j].stChnZoomAttr.enZoomType) ?
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnZoomAttr.stRect.s32Y : 0,
				(!pvoCtx->astLayerCtx[i].astChnCtx[j].stChnZoomAttr.enZoomType) ?
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnZoomAttr.stRect.u32Width : 0,
				(!pvoCtx->astLayerCtx[i].astChnCtx[j].stChnZoomAttr.enZoomType) ?
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnZoomAttr.stRect.u32Height : 0,
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnZoomAttr.enZoomType ?
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnZoomAttr.stZoomRatio.u32Xratio : 0,
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnZoomAttr.enZoomType ?
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnZoomAttr.stZoomRatio.u32Yratio : 0,
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnZoomAttr.enZoomType ?
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnZoomAttr.stZoomRatio.u32WidthRatio : 0,
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnZoomAttr.enZoomType ?
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnZoomAttr.stZoomRatio.u32HeightRatio : 0);
		}
	}

	seq_puts(m, "\n-------------------------------CHN ASPECTRATIO INFO-----------------------\n");
	seq_printf(m, "%10s%10s%15s%10s%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "ChnId", "AspectRatio", "BkClrEn", "BkClr", "RectX", "RectY", "RectW", "RectH", "Mirror");
	for (i = 0; i < VO_MAX_LAYER_NUM; ++i) {
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (!pvoCtx->astLayerCtx[i].astChnCtx[j].is_chn_enable)
				continue;
			memset(c, 0, sizeof(c));
			_chnAspectRatio_to_String(pvoCtx->astLayerCtx[i].astChnCtx[j].stChnParam.stAspectRatio.enMode, c, sizeof(c));
			memset(d, 0, sizeof(d));
			_chnMirror_to_String(pvoCtx->astLayerCtx[i].astChnCtx[j].enChnMirror, d, sizeof(d));

			seq_printf(m, "%8s%2d%8s%2d%15s%10s%10X%10d%10d%10d%10d%10s\n",
				"#",
				i,
				"#",
				j,
				c,
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnParam.stAspectRatio.bEnableBgColor ? "Y" : "N",
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnParam.stAspectRatio.u32BgColor,
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnParam.stAspectRatio.stVideoRect.s32X,
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnParam.stAspectRatio.stVideoRect.s32Y,
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnParam.stAspectRatio.stVideoRect.u32Width,
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnParam.stAspectRatio.stVideoRect.u32Height,
				d);
		}
	}

	seq_puts(m, "\n-------------------------------CHN BORDER INFO----------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "ChnId", "BorderEn", "TopW", "BottomW", "LeftW", "RightW", "Color");
	for (i = 0; i < VO_MAX_LAYER_NUM; ++i) {
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (!pvoCtx->astLayerCtx[i].astChnCtx[j].is_chn_enable)
				continue;

			seq_printf(m, "%8s%2d%8s%2d%10s%10d%10d%10d%10d%10X\n",
				"#",
				i,
				"#",
				j,
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnBorder.enable ? "Y" : "N",
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnBorder.stBorder.u32TopWidth,
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnBorder.stBorder.u32BottomWidth,
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnBorder.stBorder.u32LeftWidth,
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnBorder.stBorder.u32RightWidth,
				pvoCtx->astLayerCtx[i].astChnCtx[j].stChnBorder.stBorder.u32Color);
		}
	}

	// chn play info
	seq_puts(m, "\n-------------------------------CHN PLAY INFO------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%20s%20s%20s\n",
		"LayerId", "ChnId", "Show", "Pause", "Step", "ChnSrcFrt", "ChnFrt", "ChnGap(us)", "DispalyPts", "PreDonePts");
	for (i = 0; i < VO_MAX_LAYER_NUM; ++i) {
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (!pvoCtx->astLayerCtx[i].astChnCtx[j].is_chn_enable)
				continue;
			seq_printf(m, "%8s%2d%8s%2d%10s%10s%10s%10d%10d%20d%20lld%20lld\n",
				"#",
				i,
				"#",
				j,
				(pvoCtx->astLayerCtx[i].astChnCtx[j].bHide) ? "N" : "Y",
				(pvoCtx->astLayerCtx[i].astChnCtx[j].bPause) ? "Y" : "N",
				(pvoCtx->astLayerCtx[i].astChnCtx[j].bStep) ? "Y" : "N",
				pvoCtx->astLayerCtx[i].astChnCtx[j].u32ChnSrcFrameRate,
				pvoCtx->astLayerCtx[i].astChnCtx[j].u32ChnFrameRate,
				(pvoCtx->astLayerCtx[i].astChnCtx[j].u32ChnFrameRate == 0) ?
				0 : (1000000 / pvoCtx->astLayerCtx[i].astChnCtx[j].u32ChnFrameRate),
				pvoCtx->astLayerCtx[i].astChnCtx[j].u64DisplayPts,
				pvoCtx->astLayerCtx[i].astChnCtx[j].u64PreDonePts);
		}
	}

	// wbc info
	seq_puts(m, "\n-------------------------------WBC INFO 1---------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%15s%10s\n",
		"WbcId", "WbcEn",  "SrcType", "SrcId", "WbcW", "WbcH", "PixFmt", "Depth");
	for (i = 0; i < VO_MAX_WBC_NUM; ++i) {
		memset(c, 0, sizeof(c));
		_wbcSrc_to_String(pvoCtx->astWbcCtx[i].stWbcSrc.enSrcType, c, sizeof(c));
		memset(d, 0, sizeof(d));
		_pix_fmt_to_string(pvoCtx->astWbcCtx[i].stWbcAttr.enPixFormat, d, sizeof(d));

		seq_printf(m, "%8s%2d%10s%10s%10d%10d%10d%15s%10d\n",
				"#",
				i,
				(pvoCtx->astWbcCtx[i].is_wbc_enable) ? "Y" : "N",
				c,
				(pvoCtx->astWbcCtx[i].stWbcSrc.u32SrcId),
				pvoCtx->astWbcCtx[i].stWbcAttr.stTargetSize.u32Width,
				pvoCtx->astWbcCtx[i].stWbcAttr.stTargetSize.u32Height,
				d,
				pvoCtx->astWbcCtx[i].u32Depth);
	}

	seq_puts(m, "\n-------------------------------WBC INFO 2 (continue)----------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%20s\n",
		"WbcId", "WbcMode", "WbcFrt", "RealFrt", "OdmafifoFullCnt");
	for (i = 0; i < VO_MAX_WBC_NUM; ++i) {
		memset(c, 0, sizeof(c));
		_wbcMode_to_String(pvoCtx->astWbcCtx[i].enWbcMode, c, sizeof(c));

		seq_printf(m, "%8s%2d%10s%10d%10d%20d\n",
				"#",
				i,
				c,
				pvoCtx->astWbcCtx[i].stWbcAttr.u32FrameRate,
				pvoCtx->astWbcCtx[i].u32WbcFrameRate,
				pvoCtx->astWbcCtx[i].u32OdmaFifoFull);
	}

	return 0;
}

static int _vo_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, _vo_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops _vo_proc_fops = {
	.proc_open = _vo_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations _vo_proc_fops = {
	.owner = THIS_MODULE,
	.open = _vo_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif


int vo_proc_init(struct cvi_vo_ctx *ctx)
{
	int rc = 0;

	/* create the /proc file */
	if (proc_create_data(VO_PRC_NAME, 0644, NULL, &_vo_proc_fops, ctx) == NULL) {
		pr_err("vo proc creation failed\n");
		rc = -1;
	}

	return rc;
}

int vo_proc_remove(void)
{
	remove_proc_entry(VO_PRC_NAME, NULL);

	return 0;
}
