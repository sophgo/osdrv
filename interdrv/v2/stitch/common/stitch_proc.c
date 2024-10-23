#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include "common.h"
#include <comm_stitch.h>

#include "stitch_debug.h"
#include "stitch_proc.h"
#include <base_common.h>
// #include "stitch_common.h"
// #include "scaler.h"
#include "stitch_core.h"
#include "stitch.h"

#define STITCH_PROC_NAME "soph/stitch"

// for proc info
static int proc_stitch_mode;
static const char *const str_src[] = {"ISP", "LDC", "MEM"};
static const char *const str_sclr_flip[] = {"No", "HFLIP", "VFLIP", "HVFLIP"};
static const char *const str_sclr_odma_mode[] = {"CSC", "QUANT", "HSV", "DISABLE"};
static const char *const str_sclr_fmt[] = {"YUV420", "YUV422", "RGB_PLANAR", "RGB_PACKED", "BGR_PACKED", "Y_ONLY"};
static const char *const str_sclr_csc[] = {"Disable", "2RGB_601_Limit",
										"2RGB_601_Full", "2RGB_709_Limit",
										"2RGB_709_Full", "2YUV_601_Limit",
										"2YUV_601_Full",
										"2YUV_709_Limit", "2YUV_709_Full"};

/*************************************************************************
 *	STITCH proc functions
 *************************************************************************/
static void _pix_fmt_to_string(enum _pixel_format_e PixFmt, char *str, int len)
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

static void _data_src_to_string(enum stitch_data_src dataSrc, char *str, int len)
{
	switch (dataSrc) {
	case STITCH_DATA_SRC_DDR:
		strncpy(str, "DDR", len);
		break;
	case STITCH_DATA_SRC_VPSS:
		strncpy(str, "VPSS", len);
		break;
	case STITCH_DATA_SRC_SEP:
		strncpy(str, "SEP", len);
		break;
	default:
		strncpy(str, "DDR", len);
		break;
	}
}

static void _wgt_mode_to_string(enum stitch_wgt_mode wgtMode, char *str, int len)
{
	switch (wgtMode) {
	case STITCH_WGT_YUV_SHARE:
		strncpy(str, "YUV_SHARE", len);
		break;
	case STITCH_WGT_UV_SHARE:
		strncpy(str, "UV_SHARE", len);
		break;
	case STITCH_WGT_SEP:
		strncpy(str, "SEP", len);
		break;
	default:
		strncpy(str, "YUV_SHARE", len);
		break;
	}
}

static void _job_status_to_string(enum stitch_job_state jobStatus, char *str, int len)
{
	switch (jobStatus) {
	case STITCH_JOB_WAIT:
		strncpy(str, "WAIT", len);
		break;
	case STITCH_JOB_WORKING:
		strncpy(str, "WORKING", len);
		break;
	case STITCH_JOB_END:
		strncpy(str, "END", len);
		break;
	case STITCH_JOB_INVALID:
		strncpy(str, "INVALID", len);
		break;
	default:
		strncpy(str, "END", len);
		break;
	}
}

static void _hdl_state_to_string(enum stitch_handler_state hdlState, char *str, int len)
{
	switch (hdlState) {
	case STITCH_HANDLER_STATE_STOP:
		strncpy(str, "STOP", len);
		break;
	case STITCH_HANDLER_STATE_START:
		strncpy(str, "START", len);
		break;
	case STITCH_HANDLER_STATE_RUN:
		strncpy(str, "RUN", len);
		break;
	case STITCH_HANDLER_STATE_RUN_STAGE2:
		strncpy(str, "STAGE2", len);
		break;
	case STITCH_HANDLER_STATE_DONE:
		strncpy(str, "DONE", len);
		break;
	case STITCH_HANDLER_STATE_SUSPEND:
		strncpy(str, "SUSPEND", len);
		break;
	case STITCH_HANDLER_STATE_RESUME:
		strncpy(str, "RESUME", len);
		break;
	case STITCH_HANDLER_STATE_MAX:
		strncpy(str, "MAX", len);
		break;
	default:
		strncpy(str, "STOP", len);
		break;
	}
}

static void _dev_state_to_string(enum stitch_dev_state devState, char *str, int len)
{
	switch (devState) {
	case STITCH_DEV_STATE_IDLE:
		strncpy(str, "IDLE", len);
		break;
	case STITCH_DEV_STATE_RUNNING:
		strncpy(str, "RUNNING", len);
		break;
	case STITCH_DEV_STATE_END:
		strncpy(str, "END", len);
		break;
	default:
		strncpy(str, "END", len);
		break;
	}
}

static void _update_status_to_string(enum stitch_update_status updatstate, char *str, int len)
{
	switch (updatstate) {
	case STITCH_UPDATE_SRC:
		strncpy(str, "UPDATE SRC", len);
		break;
	case STITCH_UPDATE_CHN:
		strncpy(str, "UPDATE CHN", len);
		break;
	case STITCH_UPDATE_OP:
		strncpy(str, "UPDATE OP", len);
		break;
	case STITCH_UPDATE_WGT:
		strncpy(str, "UPDATE WGT", len);
		break;
	default:
		strncpy(str, "NO UPDATE", len);
		break;
	}
}

int stitch_ctx_proc_show(struct seq_file *m, void *v)
{
	int i, j;
	char str1[32];
	char str2[32];
	char str3[32];
	struct __stitch_ctx **stitchCtx = stitch_get_ctx();
	int stitch_grp_num = stitch_get_grp_num();
	bool *grp_used = stitch_get_grp_used();
	struct stitch_dev *dev = (struct stitch_dev *)m->private;
	unsigned char is_created;
	unsigned char is_started;

	// Module Param
	seq_printf(m, "\nModule: [STITCH], Build Time[%s]\n", UTS_VERSION);
	seq_printf(m, "IP_NUM: %d\tGRP_NUM: %d\tGRP_ID: ", STITCH_IP_NUM, stitch_grp_num);

	for (j = 0; j < STITCH_MAX_GRP_NUM; j++) {
		seq_printf(m, "grp_used%d:[%d]\n", j, grp_used[j]);
	}

	for(j = 0; j < stitch_grp_num; j++) {
		is_created =  stitchCtx[j]->is_created;
		is_started = stitchCtx[j]->is_started;
		seq_printf(m, "\n===========================================GRP[%d]: MODULE PARAM===========================================\n", j);
		seq_printf(m, "%20s%20s\n", "IP ID", "Num. of Inputs");
		if (stitchCtx[j] && is_created) {
			seq_printf(m, "%20d%20d\n", stitchCtx[j]->core_id, stitchCtx[j]->src_num);
		} else {
			seq_printf(m, "%20s%20s\n", "unknown", "unknown");
		}

		// seq_printf(m, "%25s%25s\n", "stitch_vb_source", "stitch_split_node_num");
		// seq_printf(m, "%18d%25d\n", 0, 1);

		// STITCH SRC ATTR
		//  seq_puts(m, "\n-------------------------------STITCH SRC ATTR------------------------------\n");
		seq_printf(m, "\n--------------------------------------GRP[%d]: STITCH SRC Image ATTR-------------------------------\n", j);
		seq_printf(m, "%20s%20s%20s%20s\n", "ID(Max: N)", "MaxW", "MaxH", "PixFmt");
		if (stitchCtx[j] && is_created) {
			memset(str1, 0, sizeof(str1));
			_pix_fmt_to_string(stitchCtx[j]->src_attr.fmt_in, str1, sizeof(str1));
			for (i = 0; i < stitchCtx[j]->src_num; i++) {
				seq_printf(m, "%20d%20d%20d%20s\n",
						i,
						stitchCtx[j]->src_attr.size[i].width,
						stitchCtx[j]->src_attr.size[i].height,
						str1);
			}
		}

		seq_printf(m, "\n-------------------------------GRP[%d]: STITCH SRC OVLP ATTR------------------------\n", j);
		seq_printf(m, "%20s%20s%20s\n", "ID(Max: N-1)", "ovlp_lx", "ovlp_rx");
		if (stitchCtx[j] && is_created) {
			for (i = 0; i < stitchCtx[j]->src_num - 1; i++) {
				seq_printf(m, "%20d%20d%20d\n",
						i,
						stitchCtx[j]->src_attr.ovlap_attr.ovlp_lx[i],
						stitchCtx[j]->src_attr.ovlap_attr.ovlp_rx[i]);
			}
		}

		seq_printf(m, "\n-------------------------------GRP[%d]: STITCH SRC BD ATTR------------------------\n", j);
		seq_printf(m, "%20s%20s%20ss\n", "ID(Max: N)", "ovlp_lx", "ovlp_rx");
		if (stitchCtx[j] && is_created) {
			for (i = 0; i < stitchCtx[j]->src_num; i++) {
				seq_printf(m, "%20d%20d%20d\n",
						i,
						stitchCtx[j]->src_attr.bd_attr.bd_lx[i],
						stitchCtx[j]->src_attr.bd_attr.bd_rx[i]);
			}
		}

		// STITCH WGT ATTR
		seq_printf(m, "\n--------------------------------------GRP[%d]: STITCH WGT ATTR-------------------------------------\n", j);
		seq_printf(m, "%20s%20s%20s%20s%20s\n", "ID(Max: N-1)", "MaxW", "MaxH", "PhyAddrWgt(Alpha)",
				"PhyAddrWgt(Beta)");
		if (stitchCtx[j] && is_created) {
			for (i = 0; i < stitchCtx[j]->src_num - 1; i++) {
				seq_printf(m, "%20d%20d%20d%20lld%20lld\n", i,
						stitchCtx[j]->wgt_attr.size_wgt[i].width,
						stitchCtx[j]->wgt_attr.size_wgt[i].height,
						stitchCtx[j]->wgt_attr.phy_addr_wgt[i][0],
						stitchCtx[j]->wgt_attr.phy_addr_wgt[i][1]);
			}
		}

		// STITCH CHN ATTR
		seq_printf(m, "\n-------------------------------GRP[%d]: STITCH CHN ATTR------------------------------\n", j);
		seq_printf(m, "%20s%20s%20s\n", "MaxW", "MaxH", "PixFmt");
		if (stitchCtx[j] && is_created) {
			memset(str1, 0, sizeof(str1));
			_pix_fmt_to_string(stitchCtx[j]->chn_attr.fmt_out, str1, sizeof(str1));
			seq_printf(m, "%20d%20d%20s\n",
						stitchCtx[j]->chn_attr.size.width,
						stitchCtx[j]->chn_attr.size.height,
						str1);
		}

		// STITCH OP ATTR
		seq_printf(m, "\n-------------------------------GRP[%d]: STITCH OP ATTR------------------------------\n", j);
		seq_printf(m, "%20s%20s\n", "dataSrc", "wgtMode");
		if (stitchCtx[j] && is_created) {
			memset(str1, 0, sizeof(str1));
			memset(str2, 0, sizeof(str2));
			_data_src_to_string(stitchCtx[j]->op_attr.data_src, str1, sizeof(str1));
			_wgt_mode_to_string(stitchCtx[j]->op_attr.wgt_mode, str2, sizeof(str2));
			seq_printf(m, "%20s%20s\n", str1, str2);
		}

		seq_printf(m, "\n-------------------------------GRP[%d]: STITCH Update STATUS-----------------------\n", j);
		seq_printf(m, "%20s%20s\n", "ParamUpdate", "UpdateStatus");
		if (stitchCtx[j] && is_created) {
			memset(str1, 0, sizeof(str1));
			_update_status_to_string(stitchCtx[j]->update_status, str1, sizeof(str1));
			seq_printf(m, "%20s%20s\n",
					(stitchCtx[j]->param_update) ? "Y" : "N",
					str1);
		}

		seq_printf(m, "\n-----------------------------------GRP[%d]: STITCH VB STATUS---------------------------\n", j);
		seq_printf(m, "%20s%20s\n", "Attached VBPool ID", "vb_size");
		if (stitchCtx[j] && is_created) {
			memset(str1, 0, sizeof(str1));
			memset(str1, 0, sizeof(str2));
			if (stitchCtx[j]->vb_pool == VB_INVALID_POOLID) {
				strncpy(str1, "N", sizeof(str1));
				strncpy(str2, "N", sizeof(str2));
			} else {
				snprintf(str1, sizeof(str1), "%d", stitchCtx[j]->vb_pool);
				snprintf(str2, sizeof(str2), "%d", stitchCtx[j]->vb_size);
			}
			seq_printf(m, "%20s%20s\n",
					str1,
					str2);
		}

		seq_printf(m, "\n---------------------------------GRP[%d]: STITCH HW STATUS---------------------------------\n", j);
		seq_printf(m, "%20s%20s%20s%20s\n", "HdlState", "JobStatus", "DevState", "Evt");
		if (stitchCtx[j] && is_created) {
			memset(str1, 0, sizeof(str1));
			memset(str2, 0, sizeof(str2));
			memset(str3, 0, sizeof(str3));
			_hdl_state_to_string(atomic_read(&stitchCtx[j]->hdl_state), str1, sizeof(str1));
			_job_status_to_string(atomic_read(&stitchCtx[j]->job.job_state), str2, sizeof(str2));
			_dev_state_to_string(atomic_read(&dev->state), str3, sizeof(str3));
			seq_printf(m, "%20s%20s%20s\n",
					str1,
					str2,
					str3);
		}

		seq_printf(m, "\n--------------------------------------------GRP[%d]: STITCH WORK STATUS--------------------------------------------\n", j);
		seq_printf(m, "%20s%20s%20s%20s%20s\n",
				"RecvCnt", "LostCnt", "DoneCnt", "FailRecvCnt", "bStart");
		if (stitchCtx[j] && is_created) {
			seq_printf(m, "%20d%20d%20d%20d%20s\n",
					stitchCtx[j]->work_status.recv_cnt,
					stitchCtx[j]->work_status.lost_cnt,
					stitchCtx[j]->work_status.done_cnt,
					stitchCtx[j]->work_status.fail_recv_cnt,
					(is_started) ? "Y" : "N");
		}

		seq_printf(m, "\n-------------------------------GRP[%d]: STITCH RUN TIME STATUS-------------------------------\n", j);
		seq_printf(m, "%20s%20s%20s%20s\n", "CostTime(us)", "MaxCostTime(us)",
				"HwCostTime(us)", "HwMaxCostTime(us)");
		if (stitchCtx[j] && is_created) {
			seq_printf(m, "%20u%20u%20u%20u\n",
					stitchCtx[j]->work_status.cost_time,
					stitchCtx[j]->work_status.max_cost_time,
					stitchCtx[j]->work_status.hw_cost_time,
					stitchCtx[j]->work_status.hw_max_cost_time);
		}
		seq_puts(m, "\n\n");
	}
	return 0;
}

static int stitch_proc_show(struct seq_file *m, void *v)
{
	return stitch_ctx_proc_show(m, v);
}

static ssize_t stitch_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
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

	if (kstrtoint(cProcInputdata, 10, &proc_stitch_mode))
		proc_stitch_mode = 0;

	return count;
}

static int stitch_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, stitch_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops stitch_proc_fops = {
	.proc_open = stitch_proc_open,
	.proc_read = seq_read,
	.proc_write = stitch_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations stitch_proc_fops = {
	.owner = THIS_MODULE,
	.open = stitch_proc_open,
	.read = seq_read,
	.write = stitch_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

int stitch_proc_init(struct stitch_dev *dev)
{
	struct proc_dir_entry *entry;

	entry = proc_create_data(STITCH_PROC_NAME, 0644, NULL,
							 &stitch_proc_fops, dev);
	if (!entry) {
		TRACE_STITCH(DBG_ERR, "stitch proc creation failed\n");
		return -ENOMEM;
	}

	return 0;
}

int stitch_proc_remove(struct stitch_dev *dev)
{
	remove_proc_entry(STITCH_PROC_NAME, NULL);
	return 0;
}
