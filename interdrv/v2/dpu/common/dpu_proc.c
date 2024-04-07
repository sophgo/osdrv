#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include "dpu_debug.h"
#include <linux/cvi_comm_dpu.h>
#include "../chip/soph/dpu.h"
#include <linux/cvi_common.h>
#include <linux/cvi_defines.h>
#include "dpu_proc.h"


#define DPU_SHARE_MEM_SIZE     (0x8000)
#define DPU_PROC_NAME          "soph/dpu"

// for proc info
static int proc_dpu_mode;

/*************************************************************************
 *	DPU proc functions
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

static void _mask_mode_to_string(DPU_MASK_MODE_E enMaskMode, char *str, int len)
{
	switch (enMaskMode) {
	case DPU_MASK_MODE_DEFAULT:
		strncpy(str, "7x7", len);
		break;
	case DPU_MASK_MODE_1x1:
		strncpy(str, "1x1", len);
		break;
	case DPU_MASK_MODE_3x3:
		strncpy(str, "3x3", len);
		break;
	case DPU_MASK_MODE_5x5:
		strncpy(str, "5x5", len);
		break;
	case DPU_MASK_MODE_7x7:
		strncpy(str, "7x7", len);
		break;
	default:
		strncpy(str, "Unknown Fmt", len);
		break;
	}
}

static void _dcc_dir_to_string(DPU_DCC_DIR_E enDccDir, char *str, int len)
{
	switch (enDccDir) {
	case DPU_DCC_DIR_DEFAULT:
		strncpy(str, "A12", len);
		break;
	case DPU_DCC_DIR_A12:
		strncpy(str, "A12", len);
		break;
	case DPU_DCC_DIR_A13:
		strncpy(str, "A13", len);
		break;
	case DPU_DCC_DIR_A14:
		strncpy(str, "A14", len);
		break;
	default:
		strncpy(str, "Unknown DCC_DIR Fmt", len);
		break;
	}
}

static void _depth_unit_to_string(DPU_DEPTH_UNIT_E enDpuDepthUnit, char *str, int len)
{
	switch (enDpuDepthUnit) {
	case DPU_DEPTH_UNIT_DEFAULT:
		strncpy(str, "MM", len);
		break;
	case DPU_DEPTH_UNIT_MM:
		strncpy(str, "MM", len);
		break;
	case DPU_DEPTH_UNIT_CM:
		strncpy(str, "CM", len);
		break;
	case DPU_DEPTH_UNIT_DM:
		strncpy(str, "DM", len);
		break;
	case DPU_DEPTH_UNIT_M:
		strncpy(str, "M", len);
		break;
	default:
		strncpy(str, "Unknown DEPTH_UNIT Fmt", len);
		break;
	}
}

static void _dpu_mode_to_string(DPU_MODE_E enDpuMode, char *str, int len)
{
	switch (enDpuMode) {
	case DPU_MODE_DEFAULT:
		strncpy(str, "SGBM_MUX0", len);
		break;
	case DPU_MODE_SGBM_MUX0:
		strncpy(str, "SGBM_MUX0", len);
		break;
	case DPU_MODE_SGBM_MUX1:
		strncpy(str, "SGBM_MUX1", len);
		break;
	case DPU_MODE_SGBM_MUX2:
		strncpy(str, "SGBM_MUX2", len);
		break;
	case DPU_MODE_SGBM_FGS_ONLINE_MUX0:
		strncpy(str, "ONLINE_MUX0", len);
		break;
	case DPU_MODE_SGBM_FGS_ONLINE_MUX1:
		strncpy(str, "ONLINE_MUX1", len);
		break;
	case DPU_MODE_SGBM_FGS_ONLINE_MUX2:
		strncpy(str, "ONLINE_MUX1", len);
		break;
	case DPU_MODE_FGS_MUX0:
		strncpy(str, "FGS_MUX0", len);
		break;
	case DPU_MODE_FGS_MUX1:
		strncpy(str, "FGS_MUX1", len);
		break;
	default:
		strncpy(str, "Unknown dpuMode Fmt", len);
		break;
	}
}

static void _disp_range_to_string(DPU_DISP_RANGE_E enDispRange, char *str, int len)
{
	switch (enDispRange) {
	case DPU_DISP_RANGE_DEFAULT:
		strncpy(str, "16", len);
		break;
	case DPU_DISP_RANGE_16:
		strncpy(str, "16", len);
		break;
	case DPU_DISP_RANGE_32:
		strncpy(str, "32", len);
		break;
	case DPU_DISP_RANGE_48:
		strncpy(str, "48", len);
		break;
	case DPU_DISP_RANGE_64:
		strncpy(str, "64", len);
		break;
	case DPU_DISP_RANGE_80:
		strncpy(str, "80", len);
		break;
	case DPU_DISP_RANGE_96:
		strncpy(str, "96", len);
		break;
	case DPU_DISP_RANGE_112:
		strncpy(str, "112", len);
		break;
	case DPU_DISP_RANGE_128:
		strncpy(str, "128", len);
		break;
	default:
		strncpy(str, "Unknown DISP RANGE", len);
		break;
	}
}

int dpu_ctx_proc_show(struct seq_file *m, void *v)
{
	int i, j;
	char c[50];
	char cmaskMode[50];
	char cdpuMode[50];
	char cdccDir[50];
	char cdispRange[50];
	char cdepthUnit[50];
	char is_started[10] ;
	char is_created[10] ;
	//struct cvi_dpu_dev *bdev = m->private;
	struct cvi_dpu_dev *dev = dpu_get_dev();
	struct cvi_dpu_ctx **pDpuCtx = dpu_get_shdw_ctx();

	// Module Param
	seq_printf(m, "\nModule: [DPU], Build Time[%s]\n", UTS_VERSION);
	seq_puts(m, "\n-------------------------------DPU HardWare STATUS-------------------------------\n");
	seq_printf(m, "%20s%20s\n", "Busy", "STCnt");
	//
	// DPU GRP ATTR
	seq_puts(m, "\n-------------------------------DPU GRP ATTR1------------------------------\n");
	seq_printf(m, "%20s%20s%20s%20s%20s%20s%20s%20s%20s%20s\n", "GrpID", "bStart", "bCreate", \
				"DevID","SrcFRate", "DstFRate", "LeftWidth","LeftHeight","RightWidth","RightHeight");

	for (i = 0; i < DPU_MAX_GRP_NUM; ++i) {
		if (pDpuCtx[i] && pDpuCtx[i]->isCreated) {
			// seq_printf(m, "%20s%20s%20s%20s%20s%20s%20s%20s%20s%20s\n", "GrpID", "bStart", "bCreate",
			// 	"DevID","SrcFRate", "DstFRate", "LeftWidth","LeftHeight","RightWidth","RightHeight");
			memset(c, 0, sizeof(c));
			_pix_fmt_to_string(pDpuCtx[i]->enPixelFormat, c, sizeof(c));

			if(pDpuCtx[i]->isStarted)
				strncpy(is_started, "Y", sizeof(is_started));
			else
				strncpy(is_started, "N", sizeof(is_started));;

			if(pDpuCtx[i]->isCreated)
				strncpy(is_created, "Y", sizeof(is_created));
			else
				strncpy(is_created, "N", sizeof(is_created));

			seq_printf(m, "%20d%20s%20s%20d%20d%20d%20d%20d%20d%20d\n",
				i,
				is_started,
				is_created,
				pDpuCtx[i]->u8DpuDev,
				pDpuCtx[i]->stGrpAttr.stFrameRate.s32SrcFrameRate,
				pDpuCtx[i]->stGrpAttr.stFrameRate.s32DstFrameRate,
				pDpuCtx[i]->stGrpAttr.stLeftImageSize.u32Width,
				pDpuCtx[i]->stGrpAttr.stLeftImageSize.u32Height,
				pDpuCtx[i]->stGrpAttr.stRightImageSize.u32Width,
				pDpuCtx[i]->stGrpAttr.stRightImageSize.u32Height
				);
		}
	}

	// DPU GRP ATTR
	seq_puts(m, "\n-------------------------------DPU GRP ATTR2------------------------------\n");
	seq_printf(m, "%10s%10s%10s%10s%10s%15s%20s%10s%10s%10s%10s%10s%15s%15s%15s%15s%15s%15s\n", "GrpID",
				"MaskMode", "DpuMode","DispRange","DccDir  ","DepthUnit","DispStartPos","Rshift1","Rshift2",
				"CaP1","CaP2","UniqRatio","DispShift","CensusShift","FxBaseline","FgsMaxCount","FgsMaxT","bIsBtcostOut");
	for (i = 0; i < DPU_MAX_GRP_NUM; ++i) {
		if (pDpuCtx[i] && pDpuCtx[i]->isCreated) {
			// seq_printf(m, "%10s%10s%10s%10s%10s%15s%10s%10s%10s%10s%10s%10s%15s%15s%15s%15s%15s%15s\n", "GrpID",
			// 	"MaskMode", "DpuMode","DispRange","DccDir  ","DepthUnit","DispStartPos","Rshift1","Rshift2",
			// 	"CaP1","CaP2","UniqRatio","DispShift","CensusShift","FxBaseline","FgsMaxCount","FgsMaxT","bIsBtcostOut");
			memset(cmaskMode, 0, sizeof(cmaskMode));
			memset(cdpuMode, 0, sizeof(cdpuMode));
			memset(cdccDir, 0, sizeof(cdccDir));
			memset(cdispRange, 0, sizeof(cdispRange));
			memset(cdepthUnit, 0, sizeof(cdepthUnit));

			_mask_mode_to_string(pDpuCtx[i]->stGrpAttr.enMaskMode, cmaskMode, sizeof(cmaskMode));
			_dpu_mode_to_string(pDpuCtx[i]->stGrpAttr.enDpuMode, cdpuMode, sizeof(cdpuMode));
			_dcc_dir_to_string(pDpuCtx[i]->stGrpAttr.enDccDir,cdccDir,sizeof(cdccDir));
			_depth_unit_to_string(pDpuCtx[i]->stGrpAttr.enDpuDepthUnit,cdepthUnit,sizeof(cdepthUnit));
			_disp_range_to_string(pDpuCtx[i]->stGrpAttr.enDispRange,cdispRange,sizeof(cdispRange));
			seq_printf(m, "%10d%10s%10s%10s%10s%15s%20d%10d%10d%10d%10d%10d%15d%15d%15d%15d%15d%15d\n",
				i,
				cmaskMode,
				cdpuMode,
				cdispRange,
				cdccDir,
				cdepthUnit,
				pDpuCtx[i]->stGrpAttr.u16DispStartPos,
				pDpuCtx[i]->stGrpAttr.u32Rshift1,
				pDpuCtx[i]->stGrpAttr.u32Rshift2,
				pDpuCtx[i]->stGrpAttr.u32CaP1,
				pDpuCtx[i]->stGrpAttr.u32CaP2,
				pDpuCtx[i]->stGrpAttr.u32UniqRatio,
				pDpuCtx[i]->stGrpAttr.u32DispShift,
				pDpuCtx[i]->stGrpAttr.u32CensusShift,
				pDpuCtx[i]->stGrpAttr.u32FxBaseline,
				pDpuCtx[i]->stGrpAttr.u32FgsMaxCount,
				pDpuCtx[i]->stGrpAttr.u32FgsMaxT,
				pDpuCtx[i]->stGrpAttr.bIsBtcostOut);
		}
	}

	//DPU CHN ATTR
	seq_puts(m, "\n-------------------------------DPU CHN ATTR------------------------------\n");
	seq_printf(m, "%20s%20s%20s%20s%20s\n",
							"GrpID", "ChnID", "Enable", "Width", "Height");
	for (i = 0; i < DPU_MAX_GRP_NUM; ++i) {
		if (pDpuCtx[i] && pDpuCtx[i]->isCreated) {
			for (j = 0; j < pDpuCtx[i]->chnNum; ++j) {
				char *is_enabled ="Y";
				// seq_printf(m, "%20s%20s%20s%20s%20s\n",
				// 			"GrpID", "ChnID", "Enable", "Width", "Height");
				if(pDpuCtx[i]->stChnCfgs[i].isEnabled)
					is_enabled = "Y";
				else
					is_enabled = "N";

				seq_printf(m, "%20d%20d%20s%20d%20d\n",
					i,
					j,
					is_enabled,
					pDpuCtx[i]->stChnCfgs[j].stChnAttr.stImgSize.u32Width,
					pDpuCtx[i]->stChnCfgs[j].stChnAttr.stImgSize.u32Height);
			}
		}
	}

	//DPU INPUT JOB QUEUE STATUS
	//seq_puts(m, "\n-------------------------------DPU INPUT JOB QUEUE STATUS------------------------------\n");
	// seq_printf(m, "%20s%20s%20s\n",
	// 	"GrpID", "BusyNum", "FreeNum");
	// for (i = 0; i < DPU_MAX_GRP_NUM; ++i) {
	// 	if (pDpuCtx[i] && pDpuCtx[i]->isCreated) {
	// 			seq_printf(m, "%20s%20s%20s\n",
	// 						"GrpID", "BusyNum", "FreeNum");
	// 			seq_printf(m, "%20d%20d%20d\n",
	// 				i,
	// 				pDpuCtx[i]->stInputJobStatus.BusyNum,
	// 				pDpuCtx[i]->stInputJobStatus.FreeNum);
	// 	}
	// }

	//DPU WORKING JOB QUEUE STATUS
	//seq_puts(m, "\n-------------------------------DPU WORKING JOB QUEUE STATUS------------------------------\n");
	// seq_printf(m, "%20s%20s%20s\n",
	// 	"GrpID", "BusyNum", "FreeNum");
	// for (i = 0; i < DPU_MAX_GRP_NUM; ++i) {
	// 	if (pDpuCtx[i] && pDpuCtx[i]->isCreated) {
	// 			seq_printf(m, "%20s%20s%20s\n",
	// 						"GrpID", "BusyNum", "FreeNum");
	// 			seq_printf(m, "%20d%20d%20d\n",
	// 				i,
	// 				pDpuCtx[i]->stWorkingJobStatus.BusyNum,
	// 				pDpuCtx[i]->stWorkingJobStatus.FreeNum);
	// 	}
	// }

	//DPU OUTPUT JOB QUEUE STATUS
	//seq_puts(m, "\n-------------------------------DPU OUTPUT JOB QUEUE STATUS------------------------------\n");
	// seq_printf(m, "%20s%20s%20s\n",
	// 	"GrpID", "BusyNum", "FreeNum");
	// for (i = 0; i < DPU_MAX_GRP_NUM; ++i) {
	// 	if (pDpuCtx[i] && pDpuCtx[i]->isCreated) {
	// 			seq_printf(m, "%20s%20s%20s\n",
	// 						"GrpID", "BusyNum", "FreeNum");
	// 			seq_printf(m, "%20d%20d%20d\n",
	// 				i,
	// 				pDpuCtx[i]->stOutputJobStatus.BusyNum,
	// 				pDpuCtx[i]->stOutputJobStatus.FreeNum);
	// 	}
	// }

	// DPU GRP WORK STATUS
	seq_puts(m, "\n-------------------------------DPU GRP WORK STATUS-----------------------\n");
	seq_printf(m, "%20s%20s%20s%20s%20s%20s%20s\n",
						"GrpID", "FrameNumPerSec", "StartCnt", "FailCnt", "DoneCnt",
						"CurTaskCostTm(us)","MaxTaskCostTm(us)");
	for (i = 0; i < DPU_MAX_GRP_NUM; ++i) {
		if (pDpuCtx[i] && pDpuCtx[i]->isCreated) {
			// seq_printf(m, "%20s%20s%20s%20s%20s%20s%20s\n",
			// 			"GrpID", "FrameNumPerSec", "StartCnt", "FailCnt", "DoneCnt",
			// 			"CurTaskCostTm(us)","MaxTaskCostTm(us)");
			seq_printf(m, "%20d%20d%20d%20d%20d%20d%20d\n",
				i,
				pDpuCtx[i]->stGrpWorkStatus.FrameRate,
				pDpuCtx[i]->stGrpWorkStatus.StartCnt,
				pDpuCtx[i]->stGrpWorkStatus.StartFailCnt,
				pDpuCtx[i]->stGrpWorkStatus.SendPicCnt,
				pDpuCtx[i]->stGrpWorkStatus.CurTaskCostTm,
				pDpuCtx[i]->stGrpWorkStatus.MaxTaskCostTm);
		}
	}

	// DPU Run Time STATUS
	seq_puts(m, "\n-------------------------------DPU RUN TIME STATUS-----------------------\n");
	seq_printf(m, "%20s%20s%20s%20s%20s%20s\n",
		"DevID", "CntPerSec","MaxCntPerSec","TotalIntCnt", "HwCostTm(us)", "HwMaxCostTm(us)");
			seq_printf(m, "%20d%20d%20d%20d%20d%20d\n",
				0,
				dev->stRunTimeInfo.CntPerSec,
				dev->stRunTimeInfo.MaxCntPerSec,
				dev->stRunTimeInfo.TotalIntCnt,
				dev->stRunTimeInfo.CostTm,
				dev->stRunTimeInfo.MCostTm
			);
	return 0;
}

/*************************************************************************
 *	Proc functions
 *************************************************************************/

static int dpu_proc_show(struct seq_file *m, void *v)
{
	return dpu_ctx_proc_show(m, v);
}

static ssize_t dpu_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
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

	if (kstrtoint(cProcInputdata, 10, &proc_dpu_mode))
		proc_dpu_mode = 0;

	return count;
}

static int dpu_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, dpu_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops dpu_proc_fops = {
	.proc_open = dpu_proc_open,
	.proc_read = seq_read,
	.proc_write = dpu_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations dpu_proc_fops = {
	.owner = THIS_MODULE,
	.open = dpu_proc_open,
	.read = seq_read,
	.write = dpu_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif


int dpu_proc_init(struct cvi_dpu_dev *dev)
{
	struct proc_dir_entry *entry;

	entry = proc_create_data(DPU_PROC_NAME, 0644, NULL,
				 &dpu_proc_fops, dev);
	if (!entry) {
		CVI_TRACE_DPU(CVI_DBG_ERR, "dpu proc creation failed\n");
		return -ENOMEM;
	}

	return 0;
}

int dpu_proc_remove(struct cvi_dpu_dev *dev)
{
	remove_proc_entry(DPU_PROC_NAME, NULL);
	return 0;
}
