#include "cvi_vip_vo_proc.h"

static void *vo_shared_mem;
/*************************************************************************
 *	VO proc functions
 *************************************************************************/
static void _intfType_to_String(uint32_t intfType, char *str, int len)
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

static void _intfSync_to_String(enum _VO_INTF_SYNC_E intfSync, char *str, int len)
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
	case VO_OUTPUT_USER:
		strncpy(str, "User timing", len);
		break;
	default:
		strncpy(str, "Unknown Timing", len);
		break;
	}
}

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

int vo_proc_show(struct seq_file *m, void *v)
{
	struct cvi_vo_proc_ctx *pvoCtx = NULL;
	int i, j, cnt;
	char c[32], d[32];

	pvoCtx = (struct cvi_vo_proc_ctx *)(vo_shared_mem);
	if (!pvoCtx) {
		seq_puts(m, "vo shm = NULL\n");
		return -1;
	}

	seq_printf(m, "\nModule: [VO], Build Time[%s]\n", UTS_VERSION);

	// Device Config
	seq_puts(m, "\n-------------------------------DEVICE CONFIG------------------------------\n");
	seq_printf(m, "%10s%10s%20s%20s%10s%10s\n", "DevID", "DevEn", "IntfType", "IntfSync", "BkClr", "DevFrt");
	for (i = 0; i < VO_MAX_DEV_NUM; ++i) {
		memset(c, 0, sizeof(c));
		_intfType_to_String(pvoCtx->stPubAttr[i].enIntfType, c, sizeof(c));

		memset(d, 0, sizeof(d));
		_intfSync_to_String(pvoCtx->stPubAttr[i].enIntfSync, d, sizeof(d));
		seq_printf(m, "%8s%2d%10s%20s%20s%10X%10d\n",
				"#",
				i,
				(pvoCtx->is_dev_enable[i]) ? "Y" : "N",
				c,
				d,
				pvoCtx->stPubAttr[i].u32BgColor,
				pvoCtx->stPubAttr[i].stSyncInfo.u16FrameRate);
	}

	// video layer status 1
	seq_puts(m, "\n-------------------------------VIDEO LAYER STATUS 1-----------------------\n");
	seq_printf(m, "%10s%10s%20s%10s%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "VideoEn", "PixFmt", "ImgW", "ImgH", "DispX", "DispY", "DispW", "DispH", "DispFrt");
	for (i = 0; i < VO_MAX_LAYER_NUM; ++i) {
		memset(c, 0, sizeof(c));
		_pixFmt_to_String(pvoCtx->stLayerAttr[i].enPixFormat, c, sizeof(c));

		seq_printf(m, "%8s%2d%10s%20s%10d%10d%10d%10d%10d%10d%10d\n",
				"#",
				i,
				(pvoCtx->is_layer_enable[i]) ? "Y" : "N",
				c,
				pvoCtx->stLayerAttr[i].stImageSize.u32Width,
				pvoCtx->stLayerAttr[i].stImageSize.u32Height,
				pvoCtx->stLayerAttr[i].stDispRect.s32X,
				pvoCtx->stLayerAttr[i].stDispRect.s32Y,
				pvoCtx->stLayerAttr[i].stDispRect.u32Width,
				pvoCtx->stLayerAttr[i].stDispRect.u32Height,
				pvoCtx->stLayerAttr[i].u32DispFrmRt);
	}

	// video layer status 2
	seq_puts(m, "\n-------------------------------VIDEO LAYER STATUS 2 (continue)------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "DevId", "EnChNum", "Luma", "Cont", "Hue", "Satu", "BufLen");
	for (i = 0; i < VO_MAX_LAYER_NUM; ++i) {

		cnt = 0;
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (pvoCtx->is_chn_enable[i][j])
				cnt++;
		}

		seq_printf(m, "%8s%2d%10d%10d%10d%10d%10d%10d%10d\n",
				"#",
				i,
				0,
				cnt,
				pvoCtx->proc_amp[i][PROC_AMP_BRIGHTNESS],
				pvoCtx->proc_amp[i][PROC_AMP_CONTRAST],
				pvoCtx->proc_amp[i][PROC_AMP_HUE],
				pvoCtx->proc_amp[i][PROC_AMP_SATURATION],
				pvoCtx->u32DisBufLen[i]);
	}

	// chn basic info
	seq_puts(m, "\n-------------------------------CHN BASIC INFO-----------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "ChnId", "ChnEn", "Prio", "ChnX", "ChnY", "ChnW", "ChnH", "RotAngle");
	for (i = 0; i < VO_MAX_LAYER_NUM; ++i) {
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			memset(c, 0, sizeof(c));
			if (pvoCtx->enRotation[i][j] == ROTATION_0)
				strncpy(c, "0", sizeof(c));
			else if (pvoCtx->enRotation[i][j] == ROTATION_90)
				strncpy(c, "90", sizeof(c));
			else if (pvoCtx->enRotation[i][j] == ROTATION_180)
				strncpy(c, "180", sizeof(c));
			else if (pvoCtx->enRotation[i][j] == ROTATION_270)
				strncpy(c, "270", sizeof(c));
			else
				strncpy(c, "Invalid", sizeof(c));

			seq_printf(m, "%8s%2d%8s%2d%10s%10d%10d%10d%10d%10d%10s\n",
				"#",
				i,
				"#",
				j,
				(pvoCtx->is_chn_enable[i][j]) ? "Y" : "N",
				pvoCtx->stChnAttr[i][j].u32Priority,
				pvoCtx->stChnAttr[i][j].stRect.s32X,
				pvoCtx->stChnAttr[i][j].stRect.s32Y,
				pvoCtx->stChnAttr[i][j].stRect.u32Width,
				pvoCtx->stChnAttr[i][j].stRect.u32Height,
				c);
		}
	}

	// chn play info
	seq_puts(m, "\n-------------------------------CHN PLAY INFO------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%20s%20s%20s\n",
		"LayerId", "ChnId", "Show", "Pause", "Thrshd", "ChnFrt", "ChnGap(us)", "DispPts", "PreDonePts");
	for (i = 0; i < VO_MAX_LAYER_NUM; ++i) {
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {

			seq_printf(m, "%8s%2d%8s%2d%10s%10s%10d%10d%20d%20lld%20lld\n",
				"#",
				i,
				"#",
				j,
				(pvoCtx->show[i][j]) ? "Y" : "N",
				(pvoCtx->pause[i][j]) ? "Y" : "N",
				pvoCtx->u32DisBufLen[i],
				pvoCtx->u32RealFrameRate[i][j],
				(pvoCtx->u32RealFrameRate[i][j] == 0) ?
					0 : (1000000/pvoCtx->u32RealFrameRate[i][j]),
				pvoCtx->u64DisplayPts[i][j],
				pvoCtx->u64PreDonePts[i][j]);
		}
	}

	return 0;
}

int vo_proc_init(void *shm)
{
	int rc = 0;

	vo_shared_mem = shm;
	return rc;
}

int vo_proc_remove(void)
{
	vo_shared_mem = NULL;

	return 0;
}
