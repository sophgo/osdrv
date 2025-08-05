#include <aos/cli.h>
#include <string.h>
#include <stdio.h>
#include "osal.h"
#include "comm_vo.h"
#include "vo_ctx.h"

#define VO_PRC_NAME	"soph/vo"

/*************************************************************************
 *	VO proc functions
 *************************************************************************/
static void intf_type_to_string(vo_intf_type_e intf_type, char *str, int len)
{
	switch (intf_type) {
	case VO_INTF_MIPI:
		strncpy(str, "MIPI", len);
		break;
	case VO_INTF_LVDS:
		strncpy(str, "LVDS", len);
		break;
	case VO_INTF_BT656:
		strncpy(str, "BT656", len);
		break;
	case VO_INTF_BT1120:
		strncpy(str, "BT1120", len);
		break;
	case VO_INTF_PARALLEL_RGB:
		strncpy(str, "PARALLEL_RGB", len);
		break;
	case VO_INTF_SERIAL_RGB:
		strncpy(str, "SERIAL_RGB", len);
		break;
	case VO_INTF_I80:
		strncpy(str, "I80", len);
		break;
	case VO_INTF_HW_MCU:
		strncpy(str, "HW_MCU", len);
		break;
	default:
		strncpy(str, "Unknown Type", len);
		break;
	}
}

static void intf_sync_to_string(vo_intf_sync_e intf_sync, char *str, int len)
{
	switch (intf_sync) {
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
	case VO_OUTPUT_440x1920_60:
		strncpy(str, "440x1920@60", len);
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

static void pix_fmt_to_string(pixel_format_e pixfmt, char *str, int len)
{
	switch (pixfmt) {
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

static int vo_show_status()
{
	int i, j, cnt;
	char c[32], d[32];
	struct vo_ctx *vo_ctx_p = g_vo_ctx;

#if 0//TODO: UTS_VERSION
	printf("\nModule: [VO], Build Time[%s]\n", UTS_VERSION);
#endif
	// Device Config
	printf("\n-------------------------------DEVICE CONFIG------------------------------\n");
	printf("%10s%10s%20s%20s%10s%10s\n", "DevID", "DevEn", "intf_type", "IntfSync", "BkClr", "DevFrt");
	for (i = 0; i < VO_MAX_DEV_NUM; ++i) {
		memset(c, 0, sizeof(c));
		intf_type_to_string(vo_ctx_p->dev_ctx[i].pub_attr.intf_type, c, sizeof(c));

		memset(d, 0, sizeof(d));
		intf_sync_to_string(vo_ctx_p->dev_ctx[i].pub_attr.intf_sync, d, sizeof(d));
		printf("%8s%2d%10s%20s%20s%10X%10d\n",
			"#",
			i,
			(vo_ctx_p->dev_ctx[i].is_dev_enable) ? "Y" : "N",
			c,
			d,
			vo_ctx_p->dev_ctx[i].pub_attr.bgcolor,
			vo_ctx_p->dev_ctx[i].pub_attr.sync_info.frame_rate);
	}

	// video layer status 1
	printf("\n-------------------------------VIDEO LAYER STATUS 1-----------------------\n");
	printf("%10s%10s%20s%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "VideoEn", "pixfmt", "ImgW", "ImgH", "DispX", "DispY", "DispW", "DispH");
	for (i = 0; i < VO_MAX_VIDEO_LAYER_NUM; ++i) {
		memset(c, 0, sizeof(c));
		pix_fmt_to_string(vo_ctx_p->layer_ctx[i].layer_attr.pixformat, c, sizeof(c));

		printf("%8s%2d%10s%20s%10d%10d%10d%10d%10d%10d\n",
			"#",
			i,
			(vo_ctx_p->layer_ctx[i].is_layer_enable) ? "Y" : "N",
			c,
			vo_ctx_p->layer_ctx[i].layer_attr.img_size.width,
			vo_ctx_p->layer_ctx[i].layer_attr.img_size.height,
			vo_ctx_p->layer_ctx[i].layer_attr.disp_rect.x,
			vo_ctx_p->layer_ctx[i].layer_attr.disp_rect.y,
			vo_ctx_p->layer_ctx[i].layer_attr.disp_rect.width,
			vo_ctx_p->layer_ctx[i].layer_attr.disp_rect.height);
	}

	// video layer status 2
	printf("\n-------------------------------VIDEO LAYER STATUS 2 (continue)------------\n");
	printf("%10s%10s%10s%10s%10s%10s%10s%20s%20s\n",
		"LayerId", "BindDevId", "EnChNum", "Luma", "Cont", "Hue", "Satu", "DispalyPts", "PreDonePts");
	for (i = 0; i < VO_MAX_VIDEO_LAYER_NUM; ++i) {

		cnt = 0;
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (vo_ctx_p->layer_ctx[i].chn_ctx[j].is_chn_enable)
				cnt++;
		}

		printf("%8s%2d%10d%10d%10d%10d%10d%10d%20llu%20llu\n",
			"#",
			i,
			vo_ctx_p->layer_ctx[i].bind_dev_id,
			cnt,
			vo_ctx_p->layer_ctx[i].proc_amp[PROC_AMP_BRIGHTNESS],
			vo_ctx_p->layer_ctx[i].proc_amp[PROC_AMP_CONTRAST],
			vo_ctx_p->layer_ctx[i].proc_amp[PROC_AMP_HUE],
			vo_ctx_p->layer_ctx[i].proc_amp[PROC_AMP_SATURATION],
			vo_ctx_p->layer_ctx[i].display_pts,
			vo_ctx_p->layer_ctx[i].predone_pts);
	}

	// video layer status 3
	printf("\n-------------------------------VIDEO LAYER STATUS 3 (continue)------------\n");
	printf("%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "BufLen", "SrcFrt", "RealFrt", "BwFail", "OsdBwFail");
	for (i = 0; i < VO_MAX_VIDEO_LAYER_NUM; ++i) {

		cnt = 0;
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (vo_ctx_p->layer_ctx[i].chn_ctx[j].is_chn_enable)
				cnt++;
		}

		printf("%8s%2d%10d%10d%10d%10d%10d\n",
			"#",
			i,
			vo_ctx_p->layer_ctx[i].display_buflen,
			vo_ctx_p->layer_ctx[i].src_frame_rate,
			vo_ctx_p->layer_ctx[i].frame_rate,
			vo_ctx_p->layer_ctx[i].bw_fail,
			vo_ctx_p->layer_ctx[i].vgop_bw_fail);
	}

	// gragphic layer status 1
	printf("\n-------------------------------GRAPHIC LAYER STATUS 1-----------------------\n");
	printf("%10s%10s%10s%10s\n",
		"LayerId", "GraphicEn", "BindDevId", "Prio");
	for (i = VO_MAX_VIDEO_LAYER_NUM; i < VO_MAX_LAYER_NUM; ++i) {

		printf("%8s%2d%10d%10d%10d\n",
			"#",
			i,
			vo_ctx_p->overlay_ctx[i - VO_MAX_VIDEO_LAYER_NUM].enable,
			vo_ctx_p->overlay_ctx[i - VO_MAX_VIDEO_LAYER_NUM].bind_dev_id,
			vo_ctx_p->overlay_ctx[i - VO_MAX_VIDEO_LAYER_NUM].priority);
	}

	// chn basic info
	printf("\n-------------------------------CHN BASIC INFO 1---------------------------\n");
	printf("%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"LayerId", "ChnId", "ChnEn", "Prio", "SrcW", "SrcH", "ChnX", "ChnY",
		"ChnW", "ChnH", "RotAngle", "Thrshd");
	for (i = 0; i < VO_MAX_VIDEO_LAYER_NUM; ++i) {
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (!vo_ctx_p->layer_ctx[i].chn_ctx[j].is_chn_enable)
				continue;
			memset(c, 0, sizeof(c));
			if (vo_ctx_p->layer_ctx[i].chn_ctx[j].rotation == ROTATION_0)
				strncpy(c, "0", sizeof(c));
			else if (vo_ctx_p->layer_ctx[i].chn_ctx[j].rotation == ROTATION_90)
				strncpy(c, "90", sizeof(c));
			else if (vo_ctx_p->layer_ctx[i].chn_ctx[j].rotation == ROTATION_180)
				strncpy(c, "180", sizeof(c));
			else if (vo_ctx_p->layer_ctx[i].chn_ctx[j].rotation == ROTATION_270)
				strncpy(c, "270", sizeof(c));
			else
				strncpy(c, "Invalid", sizeof(c));

			printf("%8s%2d%8s%2d%10s%10d%10d%10d%10d%10d%10d%10d%10s%10d\n",
				"#",
				i,
				"#",
				j,
				(vo_ctx_p->layer_ctx[i].chn_ctx[j].is_chn_enable) ? "Y" : "N",
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_attr.priority,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].src_width,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].src_height,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_attr.rect.x,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_attr.rect.y,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_attr.rect.width,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].chn_attr.rect.height,
				c,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].threshold);
		}
	}

	// chn play info
	printf("\n-------------------------------CHN PLAY INFO------------------------------\n");
	printf("%10s%10s%10s%10s%10s%10s%20s%20s%20s\n",
		"LayerId", "ChnId", "Show", "Pause", "ChnSrcFrt", "ChnFrt", "ChnGap(us)", "DispalyPts", "PreDonePts");
	for (i = 0; i < VO_MAX_VIDEO_LAYER_NUM; ++i) {
		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (!vo_ctx_p->layer_ctx[i].chn_ctx[j].is_chn_enable)
				continue;
			printf("%8s%2d%8s%2d%10s%10s%10d%10d%20d%20llu%20llu\n",
				"#",
				i,
				"#",
				j,
				(vo_ctx_p->layer_ctx[i].chn_ctx[j].hide) ? "N" : "Y",
				(vo_ctx_p->layer_ctx[i].chn_ctx[j].pause) ? "Y" : "N",
				vo_ctx_p->layer_ctx[i].chn_ctx[j].src_frame_rate,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].frame_rate,
				(vo_ctx_p->layer_ctx[i].chn_ctx[j].frame_rate == 0) ?
				0 : (1000000 / vo_ctx_p->layer_ctx[i].chn_ctx[j].frame_rate),
				vo_ctx_p->layer_ctx[i].chn_ctx[j].display_pts,
				vo_ctx_p->layer_ctx[i].chn_ctx[j].predone_pts);
		}
	}

	return 0;
}

static void vo_proc_show(int32_t argc, char **argv)
{
	vo_show_status();
}
ALIOS_CLI_CMD_REGISTER(vo_proc_show, proc_vo, vo info);
