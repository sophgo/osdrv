#include "proc/vi_proc.h"
#include "proc/vi_dbg_proc.h"
#include "vi_ctx.h"
#include "aos/cli.h"

#define VI_PRC_NAME	"soph/vi"

static void *vi_shared_mem;
static struct vi_dev *m_vdev;

/*************************************************************************
 *	VI proc functions
 *************************************************************************/

static void _vi_proc_show(void)
{
	struct vi_dev *vdev = m_vdev;
	struct isp_ctx *ctx = &vdev->ctx;
	struct vi_ctx *vi_proc_ctx = NULL;
	u8 i = 0, pipe = 0, chn = 0;
	char o[8], p[8];
	u8 is_rgb = 0;
	u8 raw_num = 0;
	int pos = 0;
	char *buf = NULL;

	if (!vdev) {
		vi_pr(VI_ERR, "vdev is NULL\n");
		return;
	}

	buf = osal_calloc(1, 4096);
	if (!buf) {
		vi_pr(VI_ERR, "Failed to alloc buf\n");
		return;
	}

	vi_proc_ctx = (struct vi_ctx *)(vi_shared_mem);

	pos += sprintf(buf + pos,
		       "\n-------------------------------MODULE PARAM-------------------------------------\n");
	pos += sprintf(buf + pos, "\tDetectErrFrame\tDropErrFrame\n");
	pos += sprintf(buf + pos, "\t\t%d\t\t%d\n",
		       vi_proc_ctx->mod_param.detect_err_frame, vi_proc_ctx->mod_param.drop_err_frame);

	pos += sprintf(buf + pos,
		       "\n-------------------------------VI MODE------------------------------------------\n");
	pos += sprintf(buf + pos, "\tDevID\tPrerawFE\tPostraw\t\tScaler\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (vi_proc_ctx->is_dev_enable[i]) {
			raw_num = ctx->bind_raw[i];
			pipe = ctx->isp_csi_cfg[raw_num].bind_pipe[ISP_FE_CH0];
			pos += sprintf(buf + pos, "\t%3d\t%7s\t\t%7s\t\t%7s\n", i,
				       (ctx->is_rawreplay ? "replay_be" : "online"),
				       ctx->is_offline_postraw
					   ? (ctx->is_slice_buf_on ? "slice" : "offline")
					   : "online",
				       ctx->isp_pipe_cfg[pipe].is_offline_scaler ? "offline" : "online");
		}
	}

	pos += sprintf(buf + pos,
		       "\n-------------------------------VI DEV ATTR1-------------------------------------\n");
	pos += sprintf(buf + pos, "\tDevID\tDevEn\tBindPipe\tWidth\tHeight\tIntfM\tWkM\tScanM\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (vi_proc_ctx->is_dev_enable[i]) {
			pos += sprintf(buf + pos, "\t%3d\t%3s\t%4s\t\t%4d\t%4d", i,
				       (vi_proc_ctx->is_dev_enable[i] ? "Y" : "N"), "Y",
				       vi_proc_ctx->dev_attr[i].size.width,
				       vi_proc_ctx->dev_attr[i].size.height);

			memset(o, 0, 8);
			if (vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_BT656 ||
			    vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_BT601 ||
			    vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_BT1120_STANDARD ||
			    vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_BT1120_INTERLEAVED)
				memcpy(o, "BT", sizeof("BT"));
			else if (vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_MIPI ||
				 vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_MIPI_YUV420_NORMAL ||
				 vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_MIPI_YUV420_LEGACY ||
				 vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_MIPI_YUV422)
				memcpy(o, "MIPI", sizeof("MIPI"));
			else if (vi_proc_ctx->dev_attr[i].intf_mode == VI_MODE_LVDS)
				memcpy(o, "LVDS", sizeof("LVDS"));

			memset(p, 0, 8);
			if (vi_proc_ctx->dev_attr[i].work_mode == VI_WORK_MODE_1MULTIPLEX)
				memcpy(p, "1MUX", sizeof("1MUX"));
			else if (vi_proc_ctx->dev_attr[i].work_mode == VI_WORK_MODE_2MULTIPLEX)
				memcpy(p, "2MUX", sizeof("2MUX"));
			else if (vi_proc_ctx->dev_attr[i].work_mode == VI_WORK_MODE_3MULTIPLEX)
				memcpy(p, "3MUX", sizeof("3MUX"));
			else if (vi_proc_ctx->dev_attr[i].work_mode == VI_WORK_MODE_4MULTIPLEX)
				memcpy(p, "4MUX", sizeof("4MUX"));
			else
				memcpy(p, "Other", sizeof("Other"));

			pos += sprintf(buf + pos, "\t%4s\t%4s\t%3s\n", o, p,
				       (vi_proc_ctx->dev_attr[i].scan_mode == VI_SCAN_INTERLACED) ? "I" : "P");
		}
	}

	pos += sprintf(buf + pos,
		       "\n-------------------------------VI DEV ATTR2-------------------------------------\n");
	pos += sprintf(buf + pos, "\tDevID\tAD0\tAD1\tAD2\tAD3\tSeq\tDataType\tWDRMode\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (!vi_proc_ctx->is_dev_enable[i])
			continue;

		memset(o, 0, 8);
		if (vi_proc_ctx->dev_attr[i].data_seq == VI_DATA_SEQ_VUVU)
			memcpy(o, "VUVU", sizeof("VUVU"));
		else if (vi_proc_ctx->dev_attr[i].data_seq == VI_DATA_SEQ_UVUV)
			memcpy(o, "UVUV", sizeof("UVUV"));
		else if (vi_proc_ctx->dev_attr[i].data_seq == VI_DATA_SEQ_UYVY)
			memcpy(o, "UYVY", sizeof("UYVY"));
		else if (vi_proc_ctx->dev_attr[i].data_seq == VI_DATA_SEQ_VYUY)
			memcpy(o, "VYUY", sizeof("VYUY"));
		else if (vi_proc_ctx->dev_attr[i].data_seq == VI_DATA_SEQ_YUYV)
			memcpy(o, "YUYV", sizeof("YUYV"));
		else if (vi_proc_ctx->dev_attr[i].data_seq == VI_DATA_SEQ_YVYU)
			memcpy(o, "YVYU", sizeof("YVYU"));

		is_rgb = (vi_proc_ctx->dev_attr[i].input_data_type == VI_DATA_TYPE_RGB);
		raw_num = ctx->bind_raw[i];

		pos += sprintf(buf + pos, "\t%3d\t%1d\t%1d\t%1d\t%1d\t%3s\t%4s\t\t%3s\n", i,
			       vi_proc_ctx->dev_attr[i].ad_chn_id[0],
			       vi_proc_ctx->dev_attr[i].ad_chn_id[1],
			       vi_proc_ctx->dev_attr[i].ad_chn_id[2],
			       vi_proc_ctx->dev_attr[i].ad_chn_id[3],
			       (is_rgb) ? "N/A" : o, (is_rgb) ? "RGB" : "YUV",
			       (vdev->ctx.isp_csi_cfg[raw_num].is_hdr_on) ? "WDR_2F1" : "None");
	}

	pos += sprintf(buf + pos,
		       "\n-------------------------------VI BIND ATTR-------------------------------------\n");
	pos += sprintf(buf + pos, "\tMipiId\tDevID\tPipeNum\t\tPipeId\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (!vi_proc_ctx->is_dev_enable[i])
			continue;

		pos += sprintf(buf + pos, "\t%3d\t%3d\t%3d\t\t", ctx->bind_raw[i], i,
			       vi_proc_ctx->bind_pipe_attr[i].num);
		for (pipe = 0; pipe < vi_proc_ctx->bind_pipe_attr[i].num; pipe++) {
			pos += sprintf(buf + pos, "%3d ", vi_proc_ctx->bind_pipe_attr[i].pipe_id[pipe]);
		}
		pos += sprintf(buf + pos, "\n");
	}

	pos += sprintf(buf + pos,
		       "\n-------------------------------VI DEV TIMING ATTR-------------------------------\n");
	pos += sprintf(buf + pos, "\tDevID\tDevTimingEn\tDevFrmRate\tDevWidth\tDevHeight\n");
	for (i = 0; i < VI_MAX_DEV_NUM; i++) {
		if (!vi_proc_ctx->is_dev_enable[i])
			continue;

		pos += sprintf(buf + pos, "\t%3d\t%5s\t\t%4d\t\t%5d\t\t%5d\n", i,
			       (vi_proc_ctx->timing_attr[i].enable) ? "Y" : "N",
			       vi_proc_ctx->timing_attr[i].frm_rate,
			       vi_proc_ctx->dev_attr[i].size.width,
			       vi_proc_ctx->dev_attr[i].size.height);
	}

	pos += sprintf(buf + pos,
		       "\n-------------------------------VI CHN ATTR1-------------------------------------\n");
	pos += sprintf(buf + pos,
		       "\tPipeID\tChnID\tWidth\tHeight\tMirror\tFlip\tPixFmt\tVideoFmt\tBindPool\n");
	for (pipe = 0; pipe < VI_MAX_PIPE_NUM; pipe++) {
		if (!vi_proc_ctx->is_pipe_created[pipe])
			continue;

		for (chn = 0; chn < VI_MAX_CHN_NUM; chn++) {
			if (!vi_proc_ctx->is_chn_enable[pipe][chn])
				continue;

			memset(o, 0, 8);
			if (vi_proc_ctx->chn_attr[pipe][chn].pixel_format == PIXEL_FORMAT_YUV_PLANAR_422)
				memcpy(o, "422P", sizeof("422P"));
			else if (vi_proc_ctx->chn_attr[pipe][chn].pixel_format == PIXEL_FORMAT_YUV_PLANAR_420)
				memcpy(o, "420P", sizeof("420P"));
			else if (vi_proc_ctx->chn_attr[pipe][chn].pixel_format == PIXEL_FORMAT_YUV_PLANAR_444)
				memcpy(o, "444P", sizeof("444P"));
			else if (vi_proc_ctx->chn_attr[pipe][chn].pixel_format == PIXEL_FORMAT_NV12)
				memcpy(o, "NV12", sizeof("NV12"));
			else if (vi_proc_ctx->chn_attr[pipe][chn].pixel_format == PIXEL_FORMAT_NV21)
				memcpy(o, "NV21", sizeof("NV21"));

			pos += sprintf(buf + pos, "\t%3d\t%3d\t%4d\t%4d\t%3s\t%2s\t%3s\t%6s\t\t%4d\n",
				       pipe, chn,
				       vi_proc_ctx->chn_attr[pipe][chn].size.width,
				       vi_proc_ctx->chn_attr[pipe][chn].size.height,
				       (vi_proc_ctx->chn_attr[pipe][chn].mirror) ? "Y" : "N",
				       (vi_proc_ctx->chn_attr[pipe][chn].flip) ? "Y" : "N",
				       o, "SDR8", vi_proc_ctx->chn_attr[pipe][chn].bind_vb_pool);
		}
	}

	pos += sprintf(buf + pos,
		       "\n-------------------------------VI CHN ATT2--------------------------------------\n");
	pos += sprintf(buf + pos, "\tPipeID\tChnID\tCompressMode\tDepth\tAlign\n");
	for (pipe = 0; pipe < VI_MAX_PIPE_NUM; pipe++) {
		if (!vi_proc_ctx->is_pipe_created[pipe])
			continue;

		for (chn = 0; chn < VI_MAX_CHN_NUM; chn++) {
			if (!vi_proc_ctx->is_chn_enable[pipe][chn])
				continue;

			memset(o, 0, 8);
			if (vi_proc_ctx->chn_attr[pipe][chn].compress_mode == COMPRESS_MODE_NONE)
				memcpy(o, "None", sizeof("None"));
			else
				memcpy(o, "Y", sizeof("Y"));

			pos += sprintf(buf + pos, "\t%3d\t%3d\t%4s\t\t%3d\t%3d\n", pipe, chn,
				       o, vi_proc_ctx->chn_attr[pipe][chn].depth, 32);
		}
	}

	pos += sprintf(buf + pos,
		       "\n-------------------------------VI CHN OUTPUT RESOLUTION-------------------------\n");
	pos += sprintf(buf + pos,
		       "\tDevID\tChnID\tMirror\tFlip\tWidth\tHeight\tPixFmt\tVideoFmt\tCompressMode\n");
	for (pipe = 0; pipe < VI_MAX_PIPE_NUM; pipe++) {
		if (!vi_proc_ctx->is_pipe_created[pipe])
			continue;

		for (chn = 0; chn < VI_MAX_CHN_NUM; chn++) {
			if (!vi_proc_ctx->is_chn_enable[pipe][chn])
				continue;

			memset(o, 0, 8);
			if (vi_proc_ctx->chn_attr[pipe][chn].pixel_format == PIXEL_FORMAT_YUV_PLANAR_422)
				memcpy(o, "422P", sizeof("422P"));
			else if (vi_proc_ctx->chn_attr[pipe][chn].pixel_format == PIXEL_FORMAT_YUV_PLANAR_420)
				memcpy(o, "420P", sizeof("420P"));
			else if (vi_proc_ctx->chn_attr[pipe][chn].pixel_format == PIXEL_FORMAT_YUV_PLANAR_444)
				memcpy(o, "444P", sizeof("444P"));
			else if (vi_proc_ctx->chn_attr[pipe][chn].pixel_format == PIXEL_FORMAT_NV12)
				memcpy(o, "NV12", sizeof("NV12"));
			else if (vi_proc_ctx->chn_attr[pipe][chn].pixel_format == PIXEL_FORMAT_NV21)
				memcpy(o, "NV21", sizeof("NV21"));

			memset(p, 0, 8);
			if (vi_proc_ctx->chn_attr[pipe][chn].compress_mode == COMPRESS_MODE_NONE)
				memcpy(p, "None", sizeof("None"));
			else
				memcpy(p, "Y", sizeof("Y"));

			pos += sprintf(buf + pos, "\t%3d\t%3d\t%3s\t%2s\t%4d\t%4d\t%3s\t%6s\t\t%6s\n", pipe, chn,
				       (vi_proc_ctx->chn_attr[pipe][chn].mirror) ? "Y" : "N",
				       (vi_proc_ctx->chn_attr[pipe][chn].flip) ? "Y" : "N",
				       vi_proc_ctx->chn_attr[pipe][chn].size.width,
				       vi_proc_ctx->chn_attr[pipe][chn].size.height,
				       o, "SDR8", p);
		}
	}

	pos += sprintf(buf + pos,
		       "\n-------------------------------VI CHN ROTATE INFO-------------------------------\n");
	pos += sprintf(buf + pos, "\tDevID\tChnID\tRotate\n");
	for (pipe = 0; pipe < VI_MAX_PIPE_NUM; pipe++) {
		if (!vi_proc_ctx->is_pipe_created[pipe])
			continue;

		for (chn = 0; chn < VI_MAX_CHN_NUM; chn++) {
			if (!vi_proc_ctx->is_chn_enable[pipe][chn])
				continue;

			memset(o, 0, 8);
			if (vi_proc_ctx->rotation[pipe][chn] == ROTATION_0)
				memcpy(o, "0", sizeof("0"));
			else if (vi_proc_ctx->rotation[pipe][chn] == ROTATION_90)
				memcpy(o, "90", sizeof("90"));
			else if (vi_proc_ctx->rotation[pipe][chn] == ROTATION_180)
				memcpy(o, "180", sizeof("180"));
			else if (vi_proc_ctx->rotation[pipe][chn] == ROTATION_270)
				memcpy(o, "270", sizeof("270"));
			else
				memcpy(o, "Invalid", sizeof("Invalid"));

			pos += sprintf(buf + pos, "\t%3d\t%3d\t%3s\n", pipe, chn, o);
		}
	}

	pos += sprintf(buf + pos,
		       "\n-------------------------------VI CHN EARLY INTERRUPT INFO----------------------\n");
	pos += sprintf(buf + pos, "\tPipeID\tChnID\tEnable\tLineCnt\n");
	for (pipe = 0; pipe < VI_MAX_PIPE_NUM; pipe++) {
		if (!vi_proc_ctx->is_pipe_created[pipe])
			continue;

		for (chn = 0; chn < VI_MAX_CHN_NUM; chn++) {
			if (!vi_proc_ctx->is_chn_enable[pipe][chn])
				continue;

			pos += sprintf(buf + pos, "\t%3d\t%3d\t%3s\t%4d\n", pipe, chn,
				       vi_proc_ctx->ealy_int[pipe][chn].enable ? "Y" : "N",
				       vi_proc_ctx->ealy_int[pipe][chn].line_cnt);
		}
	}

	pos += sprintf(buf + pos,
		       "\n-------------------------------VI CHN CROP INFO---------------------------------\n");
	pos += sprintf(buf + pos,
		       "\tPipeID\tChnID\tCropEn\tCoorType\tCoorX\tCoorY\tWidth\tHeight\tTrimX\tTrimY\tTrimWid\tTrimHgt\n");
	for (pipe = 0; pipe < VI_MAX_PIPE_NUM; pipe++) {
		if (!vi_proc_ctx->is_pipe_created[pipe])
			continue;

		for (chn = 0; chn < VI_MAX_CHN_NUM; chn++) {
			if (!vi_proc_ctx->is_chn_enable[pipe][chn])
				continue;

			memset(o, 0, 8);
			if (vi_proc_ctx->chn_crop[pipe][chn].crop_coordinate == VI_CROP_RATIO_COOR)
				memcpy(o, "RAT", sizeof("RAT"));
			else
				memcpy(o, "ABS", sizeof("ABS"));

			pos += sprintf(buf + pos, "\t%3d\t%3d\t%3s\t%5s\t\t%4d\t%4d\t%4d\t%4d\t%4d\t%3d\t%3d\t%4d\n",
				       pipe, chn,
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

	pos += sprintf(buf + pos,
		       "\n-------------------------------VI CHN STATUS------------------------------------\n");
	pos += sprintf(buf + pos,
		       "\tPipeID\tChnID\tEnable\tFrameRate\tIntCnt\tRecvPic\tLostFrame\tVbFail\tWidth\tHeight\n");
	for (pipe = 0; pipe < VI_MAX_PIPE_NUM; pipe++) {
		if (!vi_proc_ctx->is_pipe_created[pipe])
			continue;

		for (chn = 0; chn < VI_MAX_CHN_NUM; chn++) {
			if (!vi_proc_ctx->is_chn_enable[pipe][chn])
				continue;

			pos += sprintf(buf + pos, "\t%3d\t%3d\t%3s\t%5d\t\t%5d\t%5d\t%5d\t\t%5d\t%4d\t%4d\n", pipe, chn,
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

	osal_printk("%s\n", buf);

	osal_free(buf);
}

int vi_proc_init(struct vi_dev *_vdev, void *shm)
{
	m_vdev = _vdev;
	vi_shared_mem = shm;

	return 0;
}

int vi_proc_remove(void)
{

	m_vdev = NULL;
	vi_shared_mem = NULL;

	return 0;
}

static void vi_proc_show(int32_t argc, char **argv)
{
	_vi_proc_show();
}

ALIOS_CLI_CMD_REGISTER(vi_proc_show, proc_vi, "vi_proc");

int vi_create_proc(struct vi_dev *vdev)
{
	int ret = 0;

	if (vi_proc_init(vdev, vdev->shared_mem) < 0) {
		vi_pr(VI_ERR, "vi proc init failed\n");
		return ERR_VI_SYS_NOTREADY;
	}

	if (vi_dbg_proc_init(vdev) < 0) {
		vi_pr(VI_ERR, "vi_dbg proc init failed\n");
		return ERR_VI_SYS_NOTREADY;
	}

	return ret;
}

void vi_destroy_proc(struct vi_dev *vdev)
{
	vi_proc_remove();
	vi_dbg_proc_remove();
}
