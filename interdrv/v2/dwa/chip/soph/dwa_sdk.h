#ifndef _DWA_SDK_H_
#define _DWA_SDK_H_

#include <linux/list.h>
#include <linux/cvi_errno.h>
#include <linux/irqreturn.h>
#include "vb.h"

#define CVI_DWA_MAGIC 0xbabeface

enum dwa_op_id { DWA_OP_MESH_JOB = 0, DWA_OP_MAX };

int dwa_exec_cb(void *dev, enum ENUM_MODULES_ID caller, u32 cmd, void *arg);
int dwa_rm_cb(void);
int dwa_reg_cb(struct cvi_dwa_vdev *wdev);

void dwa_work_handle_frm_done(struct cvi_dwa_vdev *dev, struct dwa_core *core);

/* Begin a dwa job,then add task into the job,dwa will finish all the task in the job.
 *
 * @param phHandle: u64 *phHandle
 * @return Error code (0 if successful)
 */
s32 dwa_begin_job(struct cvi_dwa_vdev *wdev,
		  struct dwa_handle_data *phHandle);

/* End a job,all tasks in the job will be submmitted to dwa
 *
 * @param phHandle: u64 *phHandle
 * @return Error code (0 if successful)
 */
s32 dwa_end_job(struct cvi_dwa_vdev *wdev, u64 hHandle);

/* Cancel a job ,then all tasks in the job will not be submmitted to dwa
 *
 * @param phHandle: u64 *phHandle
 * @return Error code (0 if successful)
 */
s32 dwa_cancel_job(struct cvi_dwa_vdev *wdev, u64 hHandle);
s32 dwa_get_work_job(struct cvi_dwa_vdev *wdev, struct dwa_handle_data *data);

/* Add a rotation task to a dwa job
 *
 * @param phHandle: u64 *phHandle
 * @param pstTask: to describe what to do
 * @param enRotation: for further settings
 * @return Error code (0 if successful)
 */
s32 dwa_add_rotation_task(struct cvi_dwa_vdev *wdev, struct dwa_task_attr *attr);
s32 dwa_add_ldc_task(struct cvi_dwa_vdev *wdev, struct dwa_task_attr *attr);
s32 dwa_add_cor_task(struct cvi_dwa_vdev *wdev, struct dwa_task_attr *attr);
s32 dwa_add_warp_task(struct cvi_dwa_vdev *wdev, struct dwa_task_attr *attr);
s32 dwa_add_affine_task(struct cvi_dwa_vdev *wdev, struct dwa_task_attr *attr);
s32 dwa_get_chn_frame(struct cvi_dwa_vdev *wdev, struct dwa_identity_attr *identity
	, VIDEO_FRAME_INFO_S *pstVideoFrame, s32 s32MilliSec);

/* set meshsize for rotation only
 *
 * @param nMeshHor: mesh counts horizontal
 * @param nMeshVer: mesh counts vertical
 * @return Error code (0 if successful)
 */
s32 cvi_dwa_set_mesh_size(int nMeshHor, int nMeshVer);

int cvi_dwa_sw_init(struct cvi_dwa_vdev *wdev);
void cvi_dwa_sw_deinit(struct cvi_dwa_vdev *wdev);

s32 dwa_set_identity(struct cvi_dwa_vdev *wdev,
				  struct dwa_identity_attr *attr);

#define DWA_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_PLANAR_444) ||           \
	 (fmt == PIXEL_FORMAT_RGB_888_PLANAR) || PIXEL_FORMAT_BGR_888_PLANAR || (fmt == PIXEL_FORMAT_YUV_400))

static inline s32 dwa_check_null_ptr(const void *ptr)
{
	if (!ptr) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "NULL pointer\n");
		return CVI_ERR_DWA_NULL_PTR;
	}
	return CVI_SUCCESS;
}

static inline s32 check_dwa_format(VIDEO_FRAME_INFO_S imgIn, VIDEO_FRAME_INFO_S imgOut)
{
	if (imgIn.stVFrame.enPixelFormat !=
		imgOut.stVFrame.enPixelFormat) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "in/out pixelformat(%d-%d) mismatch\n",
			imgIn.stVFrame.enPixelFormat,
			imgOut.stVFrame.enPixelFormat);
		return CVI_ERR_DWA_ILLEGAL_PARAM;
	}
	if (!DWA_SUPPORT_FMT(imgIn.stVFrame.enPixelFormat)) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "pixelformat(%d) unsupported\n",
			imgIn.stVFrame.enPixelFormat);
		return CVI_ERR_DWA_ILLEGAL_PARAM;
	}
	return CVI_SUCCESS;
}

static inline s32 dwa_check_param_is_valid(struct cvi_dwa_vdev *wdev, struct dwa_task_attr *attr)
{
	s32 ret = CVI_SUCCESS;

	ret = dwa_check_null_ptr(wdev);
	if (ret)
		return CVI_FALSE;

	ret = dwa_check_null_ptr(attr);
	if (ret)
		return CVI_FALSE;

	ret = check_dwa_format(attr->stImgIn, attr->stImgOut);
	if (ret)
		return CVI_FALSE;

	return CVI_TRUE;
}

static inline s32 dwa_rot_check_size(ROTATION_E enRotation, const struct dwa_task_attr *pstTask)
{
	if (enRotation > ROTATION_XY_FLIP) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "invalid rotation(%d).\n", enRotation);
		return CVI_ERR_DWA_ILLEGAL_PARAM;
	}

	if (enRotation == ROTATION_90 || enRotation == ROTATION_270 || enRotation == ROTATION_XY_FLIP) {
		if (pstTask->stImgOut.stVFrame.u32Width < pstTask->stImgIn.stVFrame.u32Height) {
			CVI_TRACE_DWA(CVI_DBG_ERR, "rotation(%d) invalid: 'output width(%d) < input height(%d)'\n",
				enRotation, pstTask->stImgOut.stVFrame.u32Width,
				pstTask->stImgIn.stVFrame.u32Height);
			return CVI_ERR_DWA_ILLEGAL_PARAM;
		}
		if (pstTask->stImgOut.stVFrame.u32Height < pstTask->stImgIn.stVFrame.u32Width) {
			CVI_TRACE_DWA(CVI_DBG_ERR, "rotation(%d) invalid: 'output height(%d) < input width(%d)'\n",
				enRotation, pstTask->stImgOut.stVFrame.u32Height,
				pstTask->stImgIn.stVFrame.u32Width);
			return CVI_ERR_DWA_ILLEGAL_PARAM;
		}
	} else {
		if (pstTask->stImgOut.stVFrame.u32Width < pstTask->stImgIn.stVFrame.u32Width) {
			CVI_TRACE_DWA(CVI_DBG_ERR, "rotation(%d) invalid: 'output width(%d) < input width(%d)'\n",
				enRotation, pstTask->stImgOut.stVFrame.u32Width,
				pstTask->stImgIn.stVFrame.u32Width);
			return CVI_ERR_DWA_ILLEGAL_PARAM;
		}
		if (pstTask->stImgOut.stVFrame.u32Height < pstTask->stImgIn.stVFrame.u32Height) {
			CVI_TRACE_DWA(CVI_DBG_ERR, "rotation(%d) invalid: 'output height(%d) < input height(%d)'\n",
				enRotation, pstTask->stImgOut.stVFrame.u32Height,
				pstTask->stImgIn.stVFrame.u32Height);
			return CVI_ERR_DWA_ILLEGAL_PARAM;
		}
	}

	return CVI_SUCCESS;
}

static inline s32 dwa_cor_check_param(struct dwa_task_attr *pstTask)
{
	const FISHEYE_ATTR_S *pstFishEyeAttr = &pstTask->stFishEyeAttr;
	s32 ret = CVI_SUCCESS;
	int i;

	if (pstFishEyeAttr->bEnable) {
		if(!pstFishEyeAttr->stGridInfoAttr.Enable) {
			if (pstFishEyeAttr->u32RegionNum == 0) {
				CVI_TRACE_DWA(CVI_DBG_ERR, "RegionNum(%d) can't be 0 if enable fisheye.\n",
						pstFishEyeAttr->u32RegionNum);
				//gdc_proc_ctx->stFishEyeStatus.u32AddTaskFail++;
				return CVI_ERR_DWA_ILLEGAL_PARAM;
			}
			if (((u32)pstFishEyeAttr->s32HorOffset > pstTask->stImgIn.stVFrame.u32Width) ||
				((u32)pstFishEyeAttr->s32VerOffset > pstTask->stImgIn.stVFrame.u32Height)) {
				CVI_TRACE_DWA(CVI_DBG_ERR, "center pos(%d %d) out of frame size(%d %d).\n",
						pstFishEyeAttr->s32HorOffset, pstFishEyeAttr->s32VerOffset,
						pstTask->stImgIn.stVFrame.u32Width, pstTask->stImgIn.stVFrame.u32Height);
				//gdc_proc_ctx->stFishEyeStatus.u32AddTaskFail++;
				return CVI_ERR_DWA_ILLEGAL_PARAM;
			}
			for (i = 0; i < pstFishEyeAttr->u32RegionNum; ++i) {
				if ((pstFishEyeAttr->enMountMode == FISHEYE_WALL_MOUNT) &&
					(pstFishEyeAttr->astFishEyeRegionAttr[i].enViewMode == FISHEYE_VIEW_360_PANORAMA)) {
					CVI_TRACE_DWA(CVI_DBG_ERR, "Rgn(%d): WALL_MOUNT not support Panorama_360.\n", i);
					//gdc_proc_ctx->stFishEyeStatus.u32AddTaskFail++;
					return CVI_ERR_DWA_ILLEGAL_PARAM;
				}
				if ((pstFishEyeAttr->enMountMode == FISHEYE_CEILING_MOUNT) &&
					(pstFishEyeAttr->astFishEyeRegionAttr[i].enViewMode == FISHEYE_VIEW_180_PANORAMA)) {
					CVI_TRACE_DWA(CVI_DBG_ERR, "Rgn(%d): CEILING_MOUNT not support Panorama_180.\n", i);
					//gdc_proc_ctx->stFishEyeStatus.u32AddTaskFail++;
					return CVI_ERR_DWA_ILLEGAL_PARAM;
				}
				if ((pstFishEyeAttr->enMountMode == FISHEYE_DESKTOP_MOUNT) &&
					(pstFishEyeAttr->astFishEyeRegionAttr[i].enViewMode == FISHEYE_VIEW_180_PANORAMA)) {
					CVI_TRACE_DWA(CVI_DBG_ERR, "Rgn(%d): DESKTOP_MOUNT not support Panorama_180.\n", i);
					//gdc_proc_ctx->stFishEyeStatus.u32AddTaskFail++;
					return CVI_ERR_DWA_ILLEGAL_PARAM;
				}
			}
		}
	}

	return ret;
}

static inline s32 dwa_affine_check_param(struct dwa_task_attr *pstTask)
{
	const AFFINE_ATTR_S *pstAffineAttr = &pstTask->stAffineAttr;
	s32 ret = CVI_SUCCESS;

	if (pstAffineAttr->u32RegionNum == 0) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "u32RegionNum(%d) can't be zero.\n", pstAffineAttr->u32RegionNum);
		return CVI_ERR_DWA_ILLEGAL_PARAM;
	}

	if (pstAffineAttr->stDestSize.u32Width > pstTask->stImgOut.stVFrame.u32Width) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "dest's width(%d) can't be larger than frame's width(%d)\n",
			pstAffineAttr->stDestSize.u32Width, pstTask->stImgOut.stVFrame.u32Width);
		return CVI_ERR_DWA_ILLEGAL_PARAM;
	}
#if 0 //not float type(cflag:-mgeneral-regs-only)
	for (i = 0; i < pstAffineAttr->u32RegionNum; ++i) {
		CVI_TRACE_DWA(CVI_DBG_INFO, "u32RegionNum(%d) (%f, %f) (%f, %f) (%f, %f) (%f, %f)\n", i,
			pstAffineAttr->astRegionAttr[i][0].x, pstAffineAttr->astRegionAttr[i][0].y,
			pstAffineAttr->astRegionAttr[i][1].x, pstAffineAttr->astRegionAttr[i][1].y,
			pstAffineAttr->astRegionAttr[i][2].x, pstAffineAttr->astRegionAttr[i][2].y,
			pstAffineAttr->astRegionAttr[i][3].x, pstAffineAttr->astRegionAttr[i][3].y);
		if ((pstAffineAttr->astRegionAttr[i][0].x < 0) || (pstAffineAttr->astRegionAttr[i][0].y < 0) ||
			(pstAffineAttr->astRegionAttr[i][1].x < 0) || (pstAffineAttr->astRegionAttr[i][1].y < 0) ||
			(pstAffineAttr->astRegionAttr[i][2].x < 0) || (pstAffineAttr->astRegionAttr[i][2].y < 0) ||
			(pstAffineAttr->astRegionAttr[i][3].x < 0) || (pstAffineAttr->astRegionAttr[i][3].y < 0)) {
			CVI_TRACE_DWA(CVI_DBG_ERR, "u32RegionNum(%d) affine point can't be negative\n", i);
			return CVI_ERR_DWA_ILLEGAL_PARAM;
		}
		if ((pstAffineAttr->astRegionAttr[i][1].x < pstAffineAttr->astRegionAttr[i][0].x) ||
			(pstAffineAttr->astRegionAttr[i][3].x < pstAffineAttr->astRegionAttr[i][2].x)) {
			CVI_TRACE_DWA(CVI_DBG_ERR, "u32RegionNum(%d) point1/3's x should be bigger thant 0/2's\n", i);
			return CVI_ERR_DWA_ILLEGAL_PARAM;
		}
		if ((pstAffineAttr->astRegionAttr[i][2].y < pstAffineAttr->astRegionAttr[i][0].y) ||
			(pstAffineAttr->astRegionAttr[i][3].y < pstAffineAttr->astRegionAttr[i][1].y)) {
			CVI_TRACE_DWA(CVI_DBG_ERR, "u32RegionNum(%d) point2/3's y should be bigger thant 0/1's\n", i);
			return CVI_ERR_DWA_ILLEGAL_PARAM;
		}
	}
#endif
	return ret;
}

static inline u8 dwa_identity_is_match(GDC_IDENTITY_ATTR_S *attr_src, GDC_IDENTITY_ATTR_S *attr_dst)
{
	if ((attr_src->enModId == attr_dst->enModId)
		&& (attr_src->u32ID == attr_dst->u32ID)
		&& (strcmp(attr_src->Name, attr_dst->Name) == 0))
		return true;
	else
		return false;
}

#endif /* _DWA_SDK_H_ */
