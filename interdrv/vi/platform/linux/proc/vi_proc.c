#include <linux/version.h>
#include "proc/vi_proc.h"
#include "proc/vi_dbg_proc.h"
#include "vi_ctx.h"

#define VI_PRC_NAME	"soph/vi"

static void *vi_shared_mem;
/*************************************************************************
 *	VI proc functions
 *************************************************************************/

static int _vi_proc_show(struct seq_file *m, void *v)
{
	struct sop_vi_dev *vdev = m->private;
	struct isp_ctx *ctx = &vdev->ctx;
	struct sop_vi_ctx *vi_proc_ctx = NULL;
	u8 i = 0, pipe = 0, chn = 0;
	char o[8], p[8];
	u8 is_rgb = 0;
	u8 raw_num = 0;

	vi_proc_ctx = (struct sop_vi_ctx *)(vi_shared_mem);

	seq_puts(m, "\n-------------------------------MODULE PARAM-------------------------------------\n");
	seq_puts(m, "\tDetectErrFrame\tDropErrFrame\n");
	seq_printf(m, "\t\t%d\t\t%d\n", vi_proc_ctx->mod_param.detect_err_frame, vi_proc_ctx->mod_param.drop_err_frame);

	seq_puts(m, "\n-------------------------------VI MODE------------------------------------------\n");
	seq_puts(m, "\tDevID\tPrerawFE\tPostraw\t\tScaler\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (vi_proc_ctx->is_dev_enable[i]) {
			raw_num = ctx->bind_raw[i];
			pipe = ctx->isp_csi_cfg[raw_num].bind_pipe[ISP_FE_CH0];
			seq_printf(m, "\t%3d\t%7s\t\t%7s\t\t%7s\n", i,
				(ctx->is_rawreplay ? "replay_be" : "online"),
				ctx->is_offline_postraw ?
				(ctx->is_slice_buf_on ? "slice" : "offline") : "online",
				ctx->isp_pipe_cfg[pipe].is_offline_scaler ? "offline" : "online");
		}
	}

	seq_puts(m, "\n-------------------------------VI DEV ATTR1-------------------------------------\n");
	seq_puts(m, "\tDevID\tDevEn\tBindPipe\tWidth\tHeight\tIntfM\tWkM\tScanM\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (vi_proc_ctx->is_dev_enable[i]) {
			seq_printf(m, "\t%3d\t%3s\t%4s\t\t%4d\t%4d", i,
				(vi_proc_ctx->is_dev_enable[i] ? "Y" : "N"), "Y",
				vi_proc_ctx->dev_attr[i].size.width,
				vi_proc_ctx->dev_attr[i].size.height);

			memset(o, 0, 8);
			if (vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_BT656 ||
			    vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_BT601 ||
			    vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_BT1120_STANDARD ||
			    vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_BT1120_INTERLEAVED)
				memcpy(o, "BT", strlen("BT") + 1);
			else if (vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_MIPI ||
				 vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_MIPI_YUV420_NORMAL ||
				 vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_MIPI_YUV420_LEGACY ||
				 vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_MIPI_YUV422)
				memcpy(o, "MIPI", strlen("MIPI") + 1);
			else if (vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_LVDS)
				memcpy(o, "LVDS", strlen("LVDS") + 1);

			memset(p, 0, 8);
			if (vi_proc_ctx->dev_attr[i].work_mode == VI_WORK_MODE_1MULTIPLEX)
				memcpy(p, "1MUX", strlen("1MUX") + 1);
			else if (vi_proc_ctx->dev_attr[i].work_mode == VI_WORK_MODE_2MULTIPLEX)
				memcpy(p, "2MUX", strlen("2MUX") + 1);
			else if (vi_proc_ctx->dev_attr[i].work_mode == VI_WORK_MODE_3MULTIPLEX)
				memcpy(p, "3MUX", strlen("3MUX") + 1);
			else if (vi_proc_ctx->dev_attr[i].work_mode == VI_WORK_MODE_4MULTIPLEX)
				memcpy(p, "4MUX", strlen("4MUX") + 1);
			else
				memcpy(p, "Other", strlen("Other") + 1);

			seq_printf(m, "\t%4s\t%4s\t%3s\n", o, p,
				(vi_proc_ctx->dev_attr[i].scan_mode == VI_SCAN_INTERLACED) ? "I" : "P");
		}
	}

	seq_puts(m, "\n-------------------------------VI DEV ATTR2-------------------------------------\n");
	seq_puts(m, "\tDevID\tAD0\tAD1\tAD2\tAD3\tSeq\tDataType\tWDRMode\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (!vi_proc_ctx->is_dev_enable[i])
			continue;

		memset(o, 0, 8);
		if (vi_proc_ctx->dev_attr[i].data_seq == VI_DATA_SEQ_VUVU)
			memcpy(o, "VUVU", strlen("VUVU") + 1);
		else if (vi_proc_ctx->dev_attr[i].data_seq == VI_DATA_SEQ_UVUV)
			memcpy(o, "UVUV", strlen("UVUV") + 1);
		else if (vi_proc_ctx->dev_attr[i].data_seq == VI_DATA_SEQ_UYVY)
			memcpy(o, "UYVY", strlen("UYVY") + 1);
		else if (vi_proc_ctx->dev_attr[i].data_seq == VI_DATA_SEQ_VYUY)
			memcpy(o, "VYUY", strlen("VYUY") + 1);
		else if (vi_proc_ctx->dev_attr[i].data_seq == VI_DATA_SEQ_YUYV)
			memcpy(o, "YUYV", strlen("YUYV") + 1);
		else if (vi_proc_ctx->dev_attr[i].data_seq == VI_DATA_SEQ_YVYU)
			memcpy(o, "YVYU", strlen("YVYU") + 1);

		is_rgb = (vi_proc_ctx->dev_attr[i].input_data_type == VI_DATA_TYPE_RGB);
		raw_num = ctx->bind_raw[i];

		seq_printf(m, "\t%3d\t%1d\t%1d\t%1d\t%1d\t%3s\t%4s\t\t%3s\n", i,
			vi_proc_ctx->dev_attr[i].ad_chn_id[0],
			vi_proc_ctx->dev_attr[i].ad_chn_id[1],
			vi_proc_ctx->dev_attr[i].ad_chn_id[2],
			vi_proc_ctx->dev_attr[i].ad_chn_id[3],
			(is_rgb) ? "N/A" : o, (is_rgb) ? "RGB" : "YUV",
			(vdev->ctx.isp_csi_cfg[raw_num].is_hdr_on) ? "WDR_2F1" : "None");
	}

	seq_puts(m, "\n-------------------------------VI BIND ATTR-------------------------------------\n");
	seq_puts(m, "\tMipiId\tDevID\tPipeNum\t\tPipeId\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (!vi_proc_ctx->is_dev_enable[i])
			continue;

		seq_printf(m, "\t%3d\t%3d\t%3d\t\t", ctx->bind_raw[i], i,
			vi_proc_ctx->bind_pipe_attr[i].num);
		for (pipe = 0; pipe < vi_proc_ctx->bind_pipe_attr[i].num; pipe++) {
			seq_printf(m, "%3d ", vi_proc_ctx->bind_pipe_attr[i].pipe_id[pipe]);
		}
		seq_puts(m, "\n");
	}

	seq_puts(m, "\n-------------------------------VI DEV TIMING ATTR-------------------------------\n");
	seq_puts(m, "\tDevID\tDevTimingEn\tDevFrmRate\tDevWidth\tDevHeight\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (!vi_proc_ctx->is_dev_enable[i])
			continue;

		seq_printf(m, "\t%3d\t%5s\t\t%4d\t\t%5d\t\t%5d\n", i,
			(vi_proc_ctx->timing_attr[i].enable) ? "Y" : "N",
			vi_proc_ctx->timing_attr[i].frm_rate,
			vi_proc_ctx->dev_attr[i].size.width,
			vi_proc_ctx->dev_attr[i].size.height);
	}

	seq_puts(m, "\n-------------------------------VI CHN ATTR1-------------------------------------\n");
	seq_puts(m, "\tPipeID\tChnID\tWidth\tHeight\tMirror\tFlip\tPixFmt\tVideoFmt\tBindPool\n");
	for (pipe = 0; pipe < VI_MAX_PIPE_NUM; pipe++) {
		if (!vi_proc_ctx->is_pipe_created[pipe])
			continue;

		for (chn = 0; chn < VI_MAX_CHN_NUM; chn++) {
			if (!vi_proc_ctx->is_chn_enable[pipe][chn])
				continue;

			memset(o, 0, 8);
			if (vi_proc_ctx->chn_attr[pipe][chn].pixel_format == PIXEL_FORMAT_YUV_PLANAR_422)
				memcpy(o, "422P", strlen("422P") + 1);
			else if (vi_proc_ctx->chn_attr[pipe][chn].pixel_format == PIXEL_FORMAT_YUV_PLANAR_420)
				memcpy(o, "420P", strlen("420P") + 1);
			else if (vi_proc_ctx->chn_attr[pipe][chn].pixel_format == PIXEL_FORMAT_YUV_PLANAR_444)
				memcpy(o, "444P", strlen("444P") + 1);
			else if (vi_proc_ctx->chn_attr[pipe][chn].pixel_format == PIXEL_FORMAT_NV12)
				memcpy(o, "NV12", strlen("NV12") + 1);
			else if (vi_proc_ctx->chn_attr[pipe][chn].pixel_format == PIXEL_FORMAT_NV21)
				memcpy(o, "NV21", strlen("NV21") + 1);

			seq_printf(m, "\t%3d\t%3d\t%4d\t%4d\t%3s\t%2s\t%3s\t%6s\t\t%4d\n", pipe, chn,
				vi_proc_ctx->chn_attr[pipe][chn].size.width,
				vi_proc_ctx->chn_attr[pipe][chn].size.height,
				(vi_proc_ctx->chn_attr[pipe][chn].mirror) ? "Y" : "N",
				(vi_proc_ctx->chn_attr[pipe][chn].flip) ? "Y" : "N",
				o, "SDR8", vi_proc_ctx->chn_attr[pipe][chn].bind_vb_pool);
		}
	}

	seq_puts(m, "\n-------------------------------VI CHN ATT2--------------------------------------\n");
	seq_puts(m, "\tPipeID\tChnID\tCompressMode\tDepth\tAlign\n");
	for (pipe = 0; pipe < VI_MAX_PIPE_NUM; pipe++) {
		if (!vi_proc_ctx->is_pipe_created[pipe])
			continue;

		for (chn = 0; chn < VI_MAX_CHN_NUM; chn++) {
			if (!vi_proc_ctx->is_chn_enable[pipe][chn])
				continue;

			memset(o, 0, 8);
			if (vi_proc_ctx->chn_attr[pipe][chn].compress_mode == COMPRESS_MODE_NONE)
				memcpy(o, "None", strlen("None") + 1);
			else
				memcpy(o, "Y", strlen("Y") + 1);

			seq_printf(m, "\t%3d\t%3d\t%4s\t\t%3d\t%3d\n", pipe, chn,
				o, vi_proc_ctx->chn_attr[pipe][chn].depth, 32);
		}
	}

	seq_puts(m, "\n-------------------------------VI CHN OUTPUT RESOLUTION-------------------------\n");
	seq_puts(m, "\tDevID\tChnID\tMirror\tFlip\tWidth\tHeight\tPixFmt\tVideoFmt\tCompressMode\n");
	for (pipe = 0; pipe < VI_MAX_PIPE_NUM; pipe++) {
		if (!vi_proc_ctx->is_pipe_created[pipe])
			continue;

		for (chn = 0; chn < VI_MAX_CHN_NUM; chn++) {
			if (!vi_proc_ctx->is_chn_enable[pipe][chn])
				continue;

			memset(o, 0, 8);
			if (vi_proc_ctx->chn_attr[pipe][chn].pixel_format == PIXEL_FORMAT_YUV_PLANAR_422)
				memcpy(o, "422P", strlen("422P") + 1);
			else if (vi_proc_ctx->chn_attr[pipe][chn].pixel_format == PIXEL_FORMAT_YUV_PLANAR_420)
				memcpy(o, "420P", strlen("420P") + 1);
			else if (vi_proc_ctx->chn_attr[pipe][chn].pixel_format == PIXEL_FORMAT_YUV_PLANAR_444)
				memcpy(o, "444P", strlen("444P") + 1);
			else if (vi_proc_ctx->chn_attr[pipe][chn].pixel_format == PIXEL_FORMAT_NV12)
				memcpy(o, "NV12", strlen("NV12") + 1);
			else if (vi_proc_ctx->chn_attr[pipe][chn].pixel_format == PIXEL_FORMAT_NV21)
				memcpy(o, "NV21", strlen("NV21") + 1);

			memset(p, 0, 8);
			if (vi_proc_ctx->chn_attr[pipe][chn].compress_mode == COMPRESS_MODE_NONE)
				memcpy(p, "None", strlen("None") + 1);
			else
				memcpy(p, "Y", strlen("Y") + 1);

			seq_printf(m, "\t%3d\t%3d\t%3s\t%2s\t%4d\t%4d\t%3s\t%6s\t\t%6s\n", pipe, chn,
				(vi_proc_ctx->chn_attr[pipe][chn].mirror) ? "Y" : "N",
				(vi_proc_ctx->chn_attr[pipe][chn].flip) ? "Y" : "N",
				vi_proc_ctx->chn_attr[pipe][chn].size.width,
				vi_proc_ctx->chn_attr[pipe][chn].size.height,
				o, "SDR8", p);
		}
	}


	seq_puts(m, "\n-------------------------------VI CHN ROTATE INFO-------------------------------\n");
	seq_puts(m, "\tDevID\tChnID\tRotate\n");
	for (pipe = 0; pipe < VI_MAX_PIPE_NUM; pipe++) {
		if (!vi_proc_ctx->is_pipe_created[pipe])
			continue;

		for (chn = 0; chn < VI_MAX_CHN_NUM; chn++) {
			if (!vi_proc_ctx->is_chn_enable[pipe][chn])
				continue;

			memset(o, 0, 8);
			if (vi_proc_ctx->rotation[pipe][chn] == ROTATION_0)
				memcpy(o, "0", strlen("0") + 1);
			else if (vi_proc_ctx->rotation[pipe][chn] == ROTATION_90)
				memcpy(o, "90", strlen("90") + 1);
			else if (vi_proc_ctx->rotation[pipe][chn] == ROTATION_180)
				memcpy(o, "180", strlen("180") + 1);
			else if (vi_proc_ctx->rotation[pipe][chn] == ROTATION_270)
				memcpy(o, "270", strlen("270") + 1);
			else
				memcpy(o, "Invalid", strlen("Invalid") + 1);

			seq_printf(m, "\t%3d\t%3d\t%3s\n", pipe, chn, o);
		}
	}

	seq_puts(m, "\n-------------------------------VI CHN EARLY INTERRUPT INFO----------------------\n");
	seq_puts(m, "\tPipeID\tChnID\tEnable\tLineCnt\n");
	for (pipe = 0; pipe < VI_MAX_PIPE_NUM; pipe++) {
		if (!vi_proc_ctx->is_pipe_created[pipe])
			continue;

		for (chn = 0; chn < VI_MAX_CHN_NUM; chn++) {
			if (!vi_proc_ctx->is_chn_enable[pipe][chn])
				continue;

			seq_printf(m, "\t%3d\t%3d\t%3s\t%4d\n", pipe, chn,
				vi_proc_ctx->ealy_int[pipe][chn].enable ? "Y" : "N",
				vi_proc_ctx->ealy_int[pipe][chn].line_cnt);
		}
	}

	seq_puts(m, "\n-------------------------------VI CHN CROP INFO---------------------------------\n");
	seq_puts(m, "\tPipeID\tChnID\tCropEn\tCoorType\tCoorX\tCoorY\tWidth\tHeight\tTrimX\tTrimY\tTrimWid\tTrimHgt\n");
	for (pipe = 0; pipe < VI_MAX_PIPE_NUM; pipe++) {
		if (!vi_proc_ctx->is_pipe_created[pipe])
			continue;

		for (chn = 0; chn < VI_MAX_CHN_NUM; chn++) {
			if (!vi_proc_ctx->is_chn_enable[pipe][chn])
				continue;

			memset(o, 0, 8);
			if (vi_proc_ctx->chn_crop[pipe][chn].crop_coordinate == VI_CROP_RATIO_COOR)
				memcpy(o, "RAT", strlen("RAT") + 1);
			else
				memcpy(o, "ABS", strlen("ABS") + 1);

			seq_printf(m, "\t%3d\t%3d\t%3s\t%5s\t\t%4d\t%4d\t%4d\t%4d\t%4d\t%3d\t%3d\t%4d\n", pipe, chn,
				vi_proc_ctx->chn_crop[pipe][chn].enable ? "Y" : "N", o,
				vi_proc_ctx->chn_crop[pipe][chn].crop_rect.x,
				vi_proc_ctx->chn_crop[pipe][chn].crop_rect.y,
				vi_proc_ctx->chn_crop[pipe][chn].crop_rect.width,
				vi_proc_ctx->chn_crop[pipe][chn].crop_rect.height,
				vi_proc_ctx->chn_crop[pipe][chn].crop_rect.x,
				vi_proc_ctx->chn_crop[pipe][chn].crop_rect.y,
				vi_proc_ctx->chn_crop[pipe][chn].crop_rect.width,
				vi_proc_ctx->chn_crop[pipe][chn].crop_rect.height);
		}
	}

	seq_puts(m, "\n-------------------------------VI CHN STATUS------------------------------------\n");
	seq_puts(m, "\tPipeID\tChnID\tEnable\tFrameRate\tIntCnt\tRecvPic\tLostFrame\tVbFail\tWidth\tHeight\n");
	for (pipe = 0; pipe < VI_MAX_PIPE_NUM; pipe++) {
		if (!vi_proc_ctx->is_pipe_created[pipe])
			continue;

		for (chn = 0; chn < VI_MAX_CHN_NUM; chn++) {
			if (!vi_proc_ctx->is_chn_enable[pipe][chn])
				continue;

			seq_printf(m, "\t%3d\t%3d\t%3s\t%5d\t\t%5d\t%5d\t%5d\t\t%5d\t%4d\t%4d\n", pipe, chn,
				vi_proc_ctx->chn_status[pipe][chn].enable ? "Y" : "N",
				vi_proc_ctx->chn_status[pipe][chn].frame_rate,
				vi_proc_ctx->chn_status[pipe][chn].int_cnt,
				vi_proc_ctx->chn_status[pipe][chn].recv_pic,
				vi_proc_ctx->chn_status[pipe][chn].lost_frame,
				vi_proc_ctx->chn_status[pipe][chn].vb_fail,
				vi_proc_ctx->chn_status[pipe][chn].size.width,
				vi_proc_ctx->chn_status[pipe][chn].size.height);
		}
	}

	return 0;
}

static int _vi_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, _vi_proc_show, PDE_DATA(inode));
}
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
static const struct proc_ops _vi_proc_fops = {
	.proc_open = _vi_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations _vi_proc_fops = {
	.owner = THIS_MODULE,
	.open = _vi_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int vi_proc_init(struct sop_vi_dev *_vdev, void *shm)
{
	int rc = 0;

	/* create the /proc file */
	if (proc_create_data(VI_PRC_NAME, 0644, NULL, &_vi_proc_fops, _vdev) == NULL) {
		pr_err("vi proc creation failed\n");
		rc = -1;
	}

	vi_shared_mem = shm;
	return rc;
}

int vi_proc_remove(void)
{
	remove_proc_entry(VI_PRC_NAME, NULL);

	return 0;
}

int vi_create_proc(struct sop_vi_dev *vdev)
{
	int ret = 0;

	if (vi_proc_init(vdev, vdev->shared_mem) < 0) {
		pr_err("vi proc init failed\n");
		return ERR_VI_SYS_NOTREADY;
	}

	if (vi_dbg_proc_init(vdev) < 0) {
		pr_err("vi_dbg proc init failed\n");
		return ERR_VI_SYS_NOTREADY;
	}

	return ret;
}

void vi_destroy_proc(struct sop_vi_dev *vdev)
{
	vi_proc_remove();
	vi_dbg_proc_remove();
}
