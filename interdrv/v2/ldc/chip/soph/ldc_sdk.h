#ifndef _LDC_SDK_H_
#define _LDC_SDK_H_

#include <linux/list.h>
#include <linux/cvi_errno.h>
#include <linux/irqreturn.h>
#include "vb.h"
#include "base_cb.h"

enum ldc_op_id { LDC_OP_MESH_JOB = 0, LDC_OP_MAX };

int ldc_exec_cb(void *dev, enum ENUM_MODULES_ID caller, u32 cmd, void *arg);
int ldc_rm_cb(void);
int ldc_reg_cb(struct cvi_ldc_vdev *wdev);

void ldc_work_handle_frm_done(struct cvi_ldc_vdev *dev, struct ldc_core *core);

/* Begin a ldc job,then add task into the job,ldc will finish all the task in the job.
 *
 * @param phHandle: u64 *phHandle
 * @return Error code (0 if successful)
 */
s32 ldc_begin_job(struct cvi_ldc_vdev *wdev,
		  struct gdc_handle_data *phHandle);

/* End a job,all tasks in the job will be submmitted to ldc
 *
 * @param phHandle: u64 *phHandle
 * @return Error code (0 if successful)
 */
s32 ldc_end_job(struct cvi_ldc_vdev *wdev, u64 hHandle);

/* Cancel a job ,then all tasks in the job will not be submmitted to ldc
 *
 * @param phHandle: u64 *phHandle
 * @return Error code (0 if successful)
 */
s32 ldc_cancel_job(struct cvi_ldc_vdev *wdev, u64 hHandle);
s32 ldc_get_work_job(struct cvi_ldc_vdev *wdev, struct gdc_handle_data *data);

/* Add a rotation task to a ldc job
 *
 * @param phHandle: u64 *phHandle
 * @param pstTask: to describe what to do
 * @param enRotation: for further settings
 * @return Error code (0 if successful)
 */
s32 ldc_add_rotation_task(struct cvi_ldc_vdev *wdev, struct gdc_task_attr *attr);
s32 ldc_add_ldc_task(struct cvi_ldc_vdev *wdev, struct gdc_task_attr *attr);
s32 ldc_get_chn_frame(struct cvi_ldc_vdev *wdev, struct gdc_identity_attr *identity
	, VIDEO_FRAME_INFO_S *pstVideoFrame, s32 s32MilliSec);

s32 ldc_attach_vb_pool(VB_POOL VbPool);
s32 ldc_detach_vb_pool(void);


/* set meshsize for rotation only
 *
 * @param nMeshHor: mesh counts horizontal
 * @param nMeshVer: mesh counts vertical
 * @return Error code (0 if successful)
 */
s32 cvi_ldc_set_mesh_size(int nMeshHor, int nMeshVer);

int cvi_ldc_sw_init(struct cvi_ldc_vdev *wdev);
void cvi_ldc_sw_deinit(struct cvi_ldc_vdev *wdev);

s32 ldc_set_identity(struct cvi_ldc_vdev *wdev,
				  struct gdc_identity_attr *attr);

#define LDC_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_NV21) || (fmt == PIXEL_FORMAT_NV12) || (fmt == PIXEL_FORMAT_YUV_400))

static inline s32 LDC_CHECK_NULL_PTR(const void *ptr)
{
	if (!ptr) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "NULL pointer\n");
		return CVI_ERR_GDC_NULL_PTR;
	}
	return CVI_SUCCESS;
}

static inline s32 CHECK_LDC_FORMAT(VIDEO_FRAME_INFO_S imgIn, VIDEO_FRAME_INFO_S imgOut)
{
	if (imgIn.stVFrame.enPixelFormat !=
		imgOut.stVFrame.enPixelFormat) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "in/out pixelformat(%d-%d) mismatch\n",
			imgIn.stVFrame.enPixelFormat,
			imgOut.stVFrame.enPixelFormat);
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}
	if (!LDC_SUPPORT_FMT(imgIn.stVFrame.enPixelFormat)) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "pixelformat(%d) unsupported\n",
			imgIn.stVFrame.enPixelFormat);
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}
	return CVI_SUCCESS;
}

static inline s32 LDC_CHECK_PARAM_IS_VALID(struct cvi_ldc_vdev *wdev, struct gdc_task_attr *attr)
{
	s32 ret = CVI_SUCCESS;

	ret = LDC_CHECK_NULL_PTR(wdev);
	if (ret)
		return CVI_FALSE;

	ret = LDC_CHECK_NULL_PTR(attr);
	if (ret)
		return CVI_FALSE;

	ret = CHECK_LDC_FORMAT(attr->stImgIn, attr->stImgOut);
	if (ret)
		return CVI_FALSE;

	return CVI_TRUE;
}

static inline s32 LDC_ROT_CHECK_SIZE(ROTATION_E enRotation, const struct gdc_task_attr *pstTask)
{
	if (enRotation > ROTATION_XY_FLIP) {
		CVI_TRACE_LDC(CVI_DBG_ERR, "invalid rotation(%d).\n", enRotation);
		return CVI_ERR_GDC_ILLEGAL_PARAM;
	}

	if (enRotation == ROTATION_90 || enRotation == ROTATION_270 || enRotation == ROTATION_XY_FLIP) {
		if (pstTask->stImgOut.stVFrame.u32Width < pstTask->stImgIn.stVFrame.u32Height) {
			CVI_TRACE_LDC(CVI_DBG_ERR, "rotation(%d) invalid: 'output width(%d) < input height(%d)'\n",
				enRotation, pstTask->stImgOut.stVFrame.u32Width,
				pstTask->stImgIn.stVFrame.u32Height);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
		if (pstTask->stImgOut.stVFrame.u32Height < pstTask->stImgIn.stVFrame.u32Width) {
			CVI_TRACE_LDC(CVI_DBG_ERR, "rotation(%d) invalid: 'output height(%d) < input width(%d)'\n",
				enRotation, pstTask->stImgOut.stVFrame.u32Height,
				pstTask->stImgIn.stVFrame.u32Width);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
	} else {
		if (pstTask->stImgOut.stVFrame.u32Width < pstTask->stImgIn.stVFrame.u32Width) {
			CVI_TRACE_LDC(CVI_DBG_ERR, "rotation(%d) invalid: 'output width(%d) < input width(%d)'\n",
				enRotation, pstTask->stImgOut.stVFrame.u32Width,
				pstTask->stImgIn.stVFrame.u32Width);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
		if (pstTask->stImgOut.stVFrame.u32Height < pstTask->stImgIn.stVFrame.u32Height) {
			CVI_TRACE_LDC(CVI_DBG_ERR, "rotation(%d) invalid: 'output height(%d) < input height(%d)'\n",
				enRotation, pstTask->stImgOut.stVFrame.u32Height,
				pstTask->stImgIn.stVFrame.u32Height);
			return CVI_ERR_GDC_ILLEGAL_PARAM;
		}
	}

	return CVI_SUCCESS;
}


static inline u8 LDC_IDENTITY_IS_MATCH(GDC_IDENTITY_ATTR_S *attr_src, GDC_IDENTITY_ATTR_S *attr_dst)
{
	if ((attr_src->enModId == attr_dst->enModId)
		&& (attr_src->u32ID == attr_dst->u32ID)
		&& (strcmp(attr_src->Name, attr_dst->Name) == 0))
		return true;
	else
		return false;
}

#endif /* _LDC_SDK_H_ */
