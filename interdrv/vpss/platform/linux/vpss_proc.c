#include <linux/version.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <generated/compile.h>

#include "base_ctx.h"

#include "vpss_debug.h"
#include "vpss_core.h"
#include "osal.h"

#define VPSS_PROC_NAME          "soph/vpss"

// for proc info
static int proc_vpss_mode;
static const char * const vb_source[] = {"CommonVB", "UserVB", "UserIon"};

/*************************************************************************
 *	VPSS proc functions
 *************************************************************************/
static void _pix_fmt_to_string(pixel_format_e pix_fmt, char *str, int len)
{
	switch (pix_fmt) {
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
	struct vpss_cores *cores = (struct vpss_cores *)m->private;
	struct vpss_ctx *ctx = &cores->ctx;
	struct vpss_chn_ctx *chn_ctx;
	bool single_mode = cores->vpss_mode.mode == VPSS_MODE_SINGLE ? true : false;

	// Module Param
	seq_printf(m, "\nModule: [VPSS], Build Time[%s]\n", UTS_VERSION);
	seq_puts(m, "\n-------------------------------MODULE PARAM-------------------------------\n");
	seq_printf(m, "%25s\n", "vpss_vb_source");
	seq_printf(m, "%25s\n", vb_source[ctx->mod_param.vpss_buf_source]);

	// VPSS Mode
	seq_puts(m, "\n-------------------------------VPSS MODE----------------------------------\n");
	seq_printf(m, "%25s%15s%15s\n", "vpss_mode", "dev0", "dev1");
	seq_printf(m, "%25s%15s%15s\n", single_mode ? "single" : "dual", single_mode ? "N" :
		(cores->device[0].is_online ? "input_isp" : "input_mem"),
		cores->device[1].is_online ? "input_isp" : "input_mem");

	// VPSS GRP ATTR
	seq_puts(m, "\n-------------------------------VPSS GRP ATTR------------------------------\n");
	seq_printf(m, "%10s%10s%10s%20s%10s%10s%5s\n", "GrpID", "MaxW", "MaxH", "pix_fmt",
				"SrcFRate", "DstFRate", "dev");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (ctx->grp_ctx[i] && ctx->grp_ctx[i]->is_created) {
			osal_memset(c, 0, sizeof(c));
			_pix_fmt_to_string(ctx->grp_ctx[i]->grp_attr.pixel_format, c, sizeof(c));

			seq_printf(m, "%8s%2d%10d%10d%20s%10d%10d%5d\n",
				"#",
				i,
				ctx->grp_ctx[i]->grp_attr.w,
				ctx->grp_ctx[i]->grp_attr.h,
				c,
				ctx->grp_ctx[i]->grp_attr.frame_rate.src_frame_rate,
				ctx->grp_ctx[i]->grp_attr.frame_rate.dst_frame_rate,
				single_mode ? 1 : ctx->grp_ctx[i]->dev_id);
		}
	}

	// VPSS GRP AMP CTRL
	seq_puts(m, "\n-------------------------------VPSS GRP AMP CTRL------------------------------\n");
	seq_printf(m, "%10s%15s%13s%15s%8s\n", "GrpID", "Brightness", "Contrast", "Saturation",
				"Hue");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (ctx->grp_ctx[i] && ctx->grp_ctx[i]->is_created) {
			seq_printf(m, "%8s%2d%15d%13d%15d%8d\n",
				"#",
				i,
				ctx->grp_ctx[i]->proc_amp[PROC_AMP_BRIGHTNESS],
				ctx->grp_ctx[i]->proc_amp[PROC_AMP_CONTRAST],
				ctx->grp_ctx[i]->proc_amp[PROC_AMP_SATURATION],
				ctx->grp_ctx[i]->proc_amp[PROC_AMP_HUE]);

		}
	}

	//VPSS CHN ATTR
	seq_puts(m, "\n-------------------------------VPSS CHN ATTR------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s\n",
		"GrpID", "PhyChnID", "Enable", "MirrorEn", "FlipEn", "SrcFRate", "DstFRate");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s\n",
		"Depth", "Aspect", "videoX", "videoY", "videoW", "videoH", "BgColor");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (ctx->grp_ctx[i] && ctx->grp_ctx[i]->is_created) {
			for (j = 0; j < VPSS_MAX_CHN_NUM; ++j) {
				int32_t x, y;
				uint32_t w, h;

				memset(c, 0, sizeof(c));
				chn_ctx = &ctx->grp_ctx[i]->chn_ctxs[j];
				if (chn_ctx->chn_attr.aspect_ratio.mode == ASPECT_RATIO_NONE)
					strncpy(c, "NONE", sizeof(c));
				else if (chn_ctx->chn_attr.aspect_ratio.mode == ASPECT_RATIO_AUTO)
					strncpy(c, "AUTO", sizeof(c));
				else if (chn_ctx->chn_attr.aspect_ratio.mode
						== ASPECT_RATIO_MANUAL)
					strncpy(c, "MANUAL", sizeof(c));
				else
					strncpy(c, "Invalid", sizeof(c));

				if (chn_ctx->chn_attr.aspect_ratio.mode == ASPECT_RATIO_MANUAL) {
					x = chn_ctx->chn_attr.aspect_ratio.video_rect.x;
					y = chn_ctx->chn_attr.aspect_ratio.video_rect.y;
					w = chn_ctx->chn_attr.aspect_ratio.video_rect.width;
					h = chn_ctx->chn_attr.aspect_ratio.video_rect.height;
				} else {
					x = y = 0;
					w = h = 0;
				}

				seq_printf(m, "%8s%2d%8s%2d%10s%10s%10s%10d%10d\n%10d%10s%10d%10d%10d%10d%#10x\n",
					"#",
					i,
					"#",
					j,
					(chn_ctx->is_enabled) ? "y" : "N",
					(chn_ctx->chn_attr.mirror) ? "y" : "N",
					(chn_ctx->chn_attr.flip) ? "y" : "N",
					chn_ctx->chn_attr.frame_rate.src_frame_rate,
					chn_ctx->chn_attr.frame_rate.dst_frame_rate,
					chn_ctx->chn_attr.depth,
					c,
					x,
					y,
					w,
					h,
					chn_ctx->chn_attr.aspect_ratio.bgcolor);
			}
		}
	}

	// VPSS GRP CROP INFO
	seq_puts(m, "\n-------------------------------VPSS GRP CROP INFO-------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s\n",
		"GrpID", "CropEn", "CoorType", "CoorX", "CoorY", "Width", "Height");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (ctx->grp_ctx[i] && ctx->grp_ctx[i]->is_created) {
			seq_printf(m, "%8s%2d%10s%10s%10d%10d%10d%10d\n",
				"#",
				i,
				(ctx->grp_ctx[i]->grp_crop_info.enable) ? "y" : "N",
				(ctx->grp_ctx[i]->grp_crop_info.crop_coordinate == VPSS_CROP_RATIO_COOR)
				? "RAT" : "ABS",
				ctx->grp_ctx[i]->grp_crop_info.crop_rect.x,
				ctx->grp_ctx[i]->grp_crop_info.crop_rect.y,
				ctx->grp_ctx[i]->grp_crop_info.crop_rect.width,
				ctx->grp_ctx[i]->grp_crop_info.crop_rect.height);
		}
	}

	// VPSS CHN CROP INFO
	seq_puts(m, "\n-------------------------------VPSS CHN CROP INFO-------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s%10s%10s\n",
		"GrpID", "ChnID", "CropEn", "CoorType", "CoorX", "CoorY", "Width", "Height");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (ctx->grp_ctx[i] && ctx->grp_ctx[i]->is_created) {
			for (j = 0; j < VPSS_MAX_CHN_NUM; ++j) {
				chn_ctx = &ctx->grp_ctx[i]->chn_ctxs[j];
				seq_printf(m, "%8s%2d%8s%2d%10s%10s%10d%10d%10d%10d\n",
					"#",
					i,
					"#",
					j,
					(chn_ctx->crop_info.enable) ? "y" : "N",
					(chn_ctx->crop_info.crop_coordinate
						== VPSS_CROP_RATIO_COOR) ? "RAT" : "ABS",
					chn_ctx->crop_info.crop_rect.x,
					chn_ctx->crop_info.crop_rect.y,
					chn_ctx->crop_info.crop_rect.width,
					chn_ctx->crop_info.crop_rect.height);
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
		if (ctx->grp_ctx[i] && ctx->grp_ctx[i]->is_created) {
			seq_printf(m, "%8s%2d%10d%10d%20d%10s%20d%20d%20d%20d\n",
				"#",
				i,
				ctx->grp_ctx[i]->grp_work_status.recv_cnt,
				ctx->grp_ctx[i]->grp_work_status.lost_cnt,
				ctx->grp_ctx[i]->grp_work_status.start_fail_cnt,
				(ctx->grp_ctx[i]->is_started) ? "y" : "N",
				ctx->grp_ctx[i]->grp_work_status.cost_time,
				ctx->grp_ctx[i]->grp_work_status.max_cost_time,
				ctx->grp_ctx[i]->grp_work_status.hw_cost_time,
				ctx->grp_ctx[i]->grp_work_status.hw_max_cost_time);
		}
	}

	// VPSS CHN OUTPUT RESOLUTION
	seq_puts(m, "\n-------------------------------VPSS CHN OUTPUT RESOLUTION-----------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%20s%10s%10s%10s%10s\n",
		"GrpID", "ChnID", "Enable", "Width", "Height", "Pixfmt", "Videofmt", "VbPool", "SendOK", "FrameRate");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (ctx->grp_ctx[i] && ctx->grp_ctx[i]->is_created) {
			for (j = 0; j < VPSS_MAX_CHN_NUM; ++j) {
				memset(c, 0, sizeof(c));
				chn_ctx = &ctx->grp_ctx[i]->chn_ctxs[j];
				_pix_fmt_to_string(chn_ctx->chn_attr.pixel_format, c, sizeof(c));

				seq_printf(m, "%8s%2d%8s%2d%10s%10d%10d%20s%10s%10d%10d%10d\n",
					"#",
					i,
					"#",
					j,
					(chn_ctx->is_enabled) ? "y" : "N",
					chn_ctx->chn_attr.width,
					chn_ctx->chn_attr.height,
					c,
					(chn_ctx->chn_attr.video_format
						== VIDEO_FORMAT_LINEAR) ? "LINEAR" : "UNKNOWN",
					chn_ctx->vb_pool,
					chn_ctx->chn_work_status.send_ok,
					chn_ctx->chn_work_status.real_frame_rate);
			}
		}
	}

	// VPSS CHN ROTATE INFO
	seq_puts(m, "\n-------------------------------VPSS CHN ROTATE INFO-----------------------\n");
	seq_printf(m, "%10s%10s%10s\n", "GrpID", "ChnID", "Rotate");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (ctx->grp_ctx[i] && ctx->grp_ctx[i]->is_created) {
			for (j = 0; j < VPSS_MAX_CHN_NUM; ++j) {
				memset(c, 0, sizeof(c));
				chn_ctx = &ctx->grp_ctx[i]->chn_ctxs[j];
				if (chn_ctx->rotation == ROTATION_0)
					strncpy(c, "0", sizeof(c));
				else if (chn_ctx->rotation == ROTATION_90)
					strncpy(c, "90", sizeof(c));
				else if (chn_ctx->rotation == ROTATION_180)
					strncpy(c, "180", sizeof(c));
				else if (chn_ctx->rotation == ROTATION_270)
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
		if (ctx->grp_ctx[i] && ctx->grp_ctx[i]->is_created) {
			for (j = 0; j < VPSS_MAX_CHN_NUM; ++j) {
				chn_ctx = &ctx->grp_ctx[i]->chn_ctxs[j];
				seq_printf(m, "%8s%2d%8s%2d%10s%10s%10d%10d\n%10d%10d%10d%20d\n",
					"#",
					i,
					"#",
					j,
					(chn_ctx->ldc_attr.enable) ? "y" : "N",
					(chn_ctx->ldc_attr.attr.aspect) ? "y" : "N",
					chn_ctx->ldc_attr.attr.x_ratio,
					chn_ctx->ldc_attr.attr.y_ratio,
					chn_ctx->ldc_attr.attr.xy_ratio,
					chn_ctx->ldc_attr.attr.center_x_offset,
					chn_ctx->ldc_attr.attr.center_y_offset,
					chn_ctx->ldc_attr.attr.distortion_ratio);
			}
		}
	}

	seq_puts(m, "\n-------------------------------VPSS HW STATUS-----------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%10s\n",
		"ID", "Online", "Status", "StartCnt", "IntCnt", "DutyRatio");
	for (i = 0; i < VPSS_DEVICE_NUM; ++i) {
		int state = osal_atomic_read(&cores->device[i].state);

		memset(c, 0, sizeof(c));
		if (state == VPSS_IDLE)
			strncpy(c, "Idle", sizeof(c));
		else if (state == VPSS_RUNNING)
			strncpy(c, "Running", sizeof(c));
		else if (state == VPSS_END)
			strncpy(c, "End", sizeof(c));

		seq_printf(m, "%8s%2d%10s%10s%10d%10d%10d\n",
			"#",
			i,
			cores->device[i].is_online ? "y" : "N",
			c,
			cores->device[i].start_cnt,
			cores->device[i].int_cnt,
			cores->device[i].duty_ratio);
	}

	// VPSS Slice buffer status
	seq_puts(m, "\n-------------------------------VPSS CHN BUF WRAP ATTR---------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s\n", "GrpID", "ChnID", "Enable", "BufLine", "WrapBufSize");
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (ctx->grp_ctx[i] && ctx->grp_ctx[i]->is_created) {
			for (j = 0; j < VPSS_MAX_CHN_NUM; ++j) {
				chn_ctx = &ctx->grp_ctx[i]->chn_ctxs[j];
				if (!chn_ctx->is_enabled ||
					!chn_ctx->buf_wrap.enable)
					continue;
				seq_printf(m, "%8s%2d%8s%2d%10s%10d%10d\n",
					"#",
					i,
					"#",
					j,
					chn_ctx->buf_wrap.enable ? "Y" : "N",
					chn_ctx->buf_wrap.buf_line,
					chn_ctx->buf_wrap.wrap_buffer_size);
			}
		}
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
	char proc_input_data[32] = {'\0'};

	if (user_buf == NULL || count >= sizeof(proc_input_data)) {
		pr_err("Invalid input value\n");
		return -EINVAL;
	}

	if (copy_from_user(proc_input_data, user_buf, count)) {
		pr_err("copy_from_user fail\n");
		return -EFAULT;
	}

	if (kstrtoint(proc_input_data, 10, &proc_vpss_mode))
		proc_vpss_mode = 0;

	return count;
}

static int vpss_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, vpss_proc_show, PDE_DATA(inode));
}

static const struct proc_ops vpss_proc_fops = {
	.proc_open = vpss_proc_open,
	.proc_read = seq_read,
	.proc_write = vpss_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

int vpss_proc_init(struct vpss_cores *cores)
{
	struct proc_dir_entry *entry;

	entry = proc_create_data(VPSS_PROC_NAME, 0644, NULL,
				 &vpss_proc_fops, cores);
	if (!entry) {
		TRACE_VPSS(DBG_ERR, "vpss proc creation failed\n");
		return -ENOMEM;
	}

	return 0;
}

int vpss_proc_remove(struct vpss_cores *cores)
{
	remove_proc_entry(VPSS_PROC_NAME, NULL);
	return 0;
}
