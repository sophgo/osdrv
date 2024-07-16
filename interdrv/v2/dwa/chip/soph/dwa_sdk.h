#ifndef _DWA_SDK_H_
#define _DWA_SDK_H_

#include <linux/list.h>
#include <linux/comm_errno.h>
#include <linux/irqreturn.h>
#include "vb.h"
#include "dwa_core.h"

#define DWA_MAGIC 0xbabeface

enum dwa_op_id { DWA_OP_MESH_JOB = 0, DWA_OP_MAX };

int dwa_exec_cb(void *dev, enum enum_modules_id caller, unsigned int cmd, void *arg);
int dwa_rm_cb(void);
int dwa_reg_cb(struct dwa_vdev *wdev);

void dwa_work_handle_frm_done(struct dwa_vdev *dev, struct dwa_core *core);

/* Begin a dwa job,then add task into the job,dwa will finish all the task in the job.
 *
 * @param phandle: unsigned long long *phandle
 * @return Error code (0 if successful)
 */
int dwa_begin_job(struct dwa_vdev *wdev,
		  struct dwa_handle_data *phandle);

/* End a job,all tasks in the job will be submmitted to dwa
 *
 * @param phandle: unsigned long long *phandle
 * @return Error code (0 if successful)
 */
int dwa_end_job(struct dwa_vdev *wdev, unsigned long long handle);

/* Cancel a job ,then all tasks in the job will not be submmitted to dwa
 *
 * @param phandle: unsigned long long *phandle
 * @return Error code (0 if successful)
 */
int dwa_cancel_job(struct dwa_vdev *wdev, unsigned long long handle);
int dwa_get_work_job(struct dwa_vdev *wdev, struct dwa_handle_data *data);

/* Add a rotation task to a dwa job
 *
 * @param phandle: unsigned long long *phandle
 * @param ptask: to describe what to do
 * @param rotation: for further settings
 * @return Error code (0 if successful)
 */
int dwa_add_rotation_task(struct dwa_vdev *wdev, struct dwa_task_attr *attr);
int dwa_add_ldc_task(struct dwa_vdev *wdev, struct dwa_task_attr *attr);
int dwa_add_cor_task(struct dwa_vdev *wdev, struct dwa_task_attr *attr);
int dwa_add_warp_task(struct dwa_vdev *wdev, struct dwa_task_attr *attr);
int dwa_add_affine_task(struct dwa_vdev *wdev, struct dwa_task_attr *attr);
int dwa_get_chn_frame(struct dwa_vdev *wdev, struct dwa_identity_attr *identity
	, video_frame_info_s *pstvideo_frame, int s32milli_sec);

/* set meshsize for rotation only
 *
 * @param nMeshHor: mesh counts horizontal
 * @param nMeshVer: mesh counts vertical
 * @return Error code (0 if successful)
 */
int dwa_set_mesh_size(int nMeshHor, int nMeshVer);

int dwa_sw_init(struct dwa_vdev *wdev);
void dwa_sw_deinit(struct dwa_vdev *wdev);

int dwa_set_identity(struct dwa_vdev *wdev,
				  struct dwa_identity_attr *attr);
int dwa_suspend_handler(void);
int dwa_resume_handler(void);

#define DWA_SUPPORT_FMT(fmt) \
	((fmt == PIXEL_FORMAT_YUV_PLANAR_420) || (fmt == PIXEL_FORMAT_YUV_PLANAR_444) ||           \
	 (fmt == PIXEL_FORMAT_RGB_888_PLANAR) || PIXEL_FORMAT_BGR_888_PLANAR || (fmt == PIXEL_FORMAT_YUV_400))

static inline int dwa_check_null_ptr(const void *ptr)
{
	if (!ptr) {
		TRACE_DWA(DBG_ERR, "NULL pointer\n");
		return ERR_DWA_NULL_PTR;
	}
	return 0;
}

static inline int check_dwa_format(video_frame_info_s imgIn, video_frame_info_s imgOut)
{
	if (imgIn.video_frame.pixel_format !=
		imgOut.video_frame.pixel_format) {
		TRACE_DWA(DBG_ERR, "in/out pixelformat(%d-%d) mismatch\n",
			imgIn.video_frame.pixel_format,
			imgOut.video_frame.pixel_format);
		return ERR_DWA_ILLEGAL_PARAM;
	}
	if (!DWA_SUPPORT_FMT(imgIn.video_frame.pixel_format)) {
		TRACE_DWA(DBG_ERR, "pixelformat(%d) unsupported\n",
			imgIn.video_frame.pixel_format);
		return ERR_DWA_ILLEGAL_PARAM;
	}
	return 0;
}

static inline int dwa_check_param_is_valid(struct dwa_vdev *wdev, struct dwa_task_attr *attr)
{
	int ret = 0;

	ret = dwa_check_null_ptr(wdev);
	if (ret)
		return false;

	ret = dwa_check_null_ptr(attr);
	if (ret)
		return false;

	ret = check_dwa_format(attr->img_in, attr->img_out);
	if (ret)
		return false;

	return true;
}

static inline int dwa_rot_check_size(rotation_e rotation, const struct dwa_task_attr *ptask)
{
	if (rotation > ROTATION_XY_FLIP) {
		TRACE_DWA(DBG_ERR, "invalid rotation(%d).\n", rotation);
		return ERR_DWA_ILLEGAL_PARAM;
	}

	if (rotation == ROTATION_90 || rotation == ROTATION_270 || rotation == ROTATION_XY_FLIP) {
		if (ptask->img_out.video_frame.width < ptask->img_in.video_frame.height) {
			TRACE_DWA(DBG_ERR, "rotation(%d) invalid: 'output width(%d) < input height(%d)'\n",
				rotation, ptask->img_out.video_frame.width,
				ptask->img_in.video_frame.height);
			return ERR_DWA_ILLEGAL_PARAM;
		}
		if (ptask->img_out.video_frame.height < ptask->img_in.video_frame.width) {
			TRACE_DWA(DBG_ERR, "rotation(%d) invalid: 'output height(%d) < input width(%d)'\n",
				rotation, ptask->img_out.video_frame.height,
				ptask->img_in.video_frame.width);
			return ERR_DWA_ILLEGAL_PARAM;
		}
	} else {
		if (ptask->img_out.video_frame.width < ptask->img_in.video_frame.width) {
			TRACE_DWA(DBG_ERR, "rotation(%d) invalid: 'output width(%d) < input width(%d)'\n",
				rotation, ptask->img_out.video_frame.width,
				ptask->img_in.video_frame.width);
			return ERR_DWA_ILLEGAL_PARAM;
		}
		if (ptask->img_out.video_frame.height < ptask->img_in.video_frame.height) {
			TRACE_DWA(DBG_ERR, "rotation(%d) invalid: 'output height(%d) < input height(%d)'\n",
				rotation, ptask->img_out.video_frame.height,
				ptask->img_in.video_frame.height);
			return ERR_DWA_ILLEGAL_PARAM;
		}
	}

	return 0;
}

static inline int dwa_cor_check_param(struct dwa_task_attr *ptask)
{
	const fisheye_attr_s *pfisheye_attr = &ptask->fisheye_attr;
	int ret = 0;
	int i;

	if (pfisheye_attr->enable) {
		if(!pfisheye_attr->grid_info_attr.enable) {
			if (pfisheye_attr->region_num == 0) {
				TRACE_DWA(DBG_ERR, "RegionNum(%d) can't be 0 if enable fisheye.\n",
						pfisheye_attr->region_num);
				//gdc_proc_ctx->fisheye_status.add_task_fail++;
				return ERR_DWA_ILLEGAL_PARAM;
			}
			if (((u32)pfisheye_attr->hor_offset > ptask->img_in.video_frame.width) ||
				((u32)pfisheye_attr->mount_mode > ptask->img_in.video_frame.height)) {
				TRACE_DWA(DBG_ERR, "center pos(%d %d) out of frame size(%d %d).\n",
						pfisheye_attr->hor_offset, pfisheye_attr->mount_mode,
						ptask->img_in.video_frame.width, ptask->img_in.video_frame.height);
				//gdc_proc_ctx->fisheye_status.add_task_fail++;
				return ERR_DWA_ILLEGAL_PARAM;
			}
			for (i = 0; i < pfisheye_attr->region_num; ++i) {
				if ((pfisheye_attr->mount_mode == FISHEYE_WALL_MOUNT) &&
					(pfisheye_attr->fisheye_region_attr[i].view_mode == FISHEYE_VIEW_360_PANORAMA)) {
					TRACE_DWA(DBG_ERR, "Rgn(%d): WALL_MOUNT not support Panorama_360.\n", i);
					//gdc_proc_ctx->fisheye_status.add_task_fail++;
					return ERR_DWA_ILLEGAL_PARAM;
				}
				if ((pfisheye_attr->mount_mode == FISHEYE_CEILING_MOUNT) &&
					(pfisheye_attr->fisheye_region_attr[i].view_mode == FISHEYE_VIEW_180_PANORAMA)) {
					TRACE_DWA(DBG_ERR, "Rgn(%d): CEILING_MOUNT not support Panorama_180.\n", i);
					//gdc_proc_ctx->fisheye_status.add_task_fail++;
					return ERR_DWA_ILLEGAL_PARAM;
				}
				if ((pfisheye_attr->mount_mode == FISHEYE_DESKTOP_MOUNT) &&
					(pfisheye_attr->fisheye_region_attr[i].view_mode == FISHEYE_VIEW_180_PANORAMA)) {
					TRACE_DWA(DBG_ERR, "Rgn(%d): DESKTOP_MOUNT not support Panorama_180.\n", i);
					//gdc_proc_ctx->fisheye_status.add_task_fail++;
					return ERR_DWA_ILLEGAL_PARAM;
				}
			}
		}
	}

	return ret;
}

static inline int dwa_affine_check_param(struct dwa_task_attr *ptask)
{
	const affine_attr_s *paffine_attr = &ptask->affine_attr;
	int ret = 0;

	if (paffine_attr->region_num == 0) {
		TRACE_DWA(DBG_ERR, "region_num(%d) can't be zero.\n", paffine_attr->region_num);
		return ERR_DWA_ILLEGAL_PARAM;
	}

	if (paffine_attr->dest_size.width > ptask->img_out.video_frame.width) {
		TRACE_DWA(DBG_ERR, "dest's width(%d) can't be larger than frame's width(%d)\n",
			paffine_attr->dest_size.width, ptask->img_out.video_frame.width);
		return ERR_DWA_ILLEGAL_PARAM;
	}
#if 0 //not float type(cflag:-mgeneral-regs-only)
	for (i = 0; i < paffine_attr->region_num; ++i) {
		TRACE_DWA(DBG_INFO, "region_num(%d) (%f, %f) (%f, %f) (%f, %f) (%f, %f)\n", i,
			paffine_attr->astRegionAttr[i][0].x, paffine_attr->astRegionAttr[i][0].y,
			paffine_attr->astRegionAttr[i][1].x, paffine_attr->astRegionAttr[i][1].y,
			paffine_attr->astRegionAttr[i][2].x, paffine_attr->astRegionAttr[i][2].y,
			paffine_attr->astRegionAttr[i][3].x, paffine_attr->astRegionAttr[i][3].y);
		if ((paffine_attr->astRegionAttr[i][0].x < 0) || (paffine_attr->astRegionAttr[i][0].y < 0) ||
			(paffine_attr->astRegionAttr[i][1].x < 0) || (paffine_attr->astRegionAttr[i][1].y < 0) ||
			(paffine_attr->astRegionAttr[i][2].x < 0) || (paffine_attr->astRegionAttr[i][2].y < 0) ||
			(paffine_attr->astRegionAttr[i][3].x < 0) || (paffine_attr->astRegionAttr[i][3].y < 0)) {
			TRACE_DWA(DBG_ERR, "region_num(%d) affine point can't be negative\n", i);
			return ERR_DWA_ILLEGAL_PARAM;
		}
		if ((paffine_attr->astRegionAttr[i][1].x < paffine_attr->astRegionAttr[i][0].x) ||
			(paffine_attr->astRegionAttr[i][3].x < paffine_attr->astRegionAttr[i][2].x)) {
			TRACE_DWA(DBG_ERR, "region_num(%d) point1/3's x should be bigger thant 0/2's\n", i);
			return ERR_DWA_ILLEGAL_PARAM;
		}
		if ((paffine_attr->astRegionAttr[i][2].y < paffine_attr->astRegionAttr[i][0].y) ||
			(paffine_attr->astRegionAttr[i][3].y < paffine_attr->astRegionAttr[i][1].y)) {
			TRACE_DWA(DBG_ERR, "region_num(%d) point2/3's y should be bigger thant 0/1's\n", i);
			return ERR_DWA_ILLEGAL_PARAM;
		}
	}
#endif
	return ret;
}

static inline unsigned char dwa_identity_is_match(gdc_identity_attr_s *attr_src, gdc_identity_attr_s *attr_dst)
{
	if ((attr_src->mod_id == attr_dst->mod_id)
		&& (attr_src->id == attr_dst->id)
		&& (strcmp(attr_src->name, attr_dst->name) == 0))
		return true;
	else
		return false;
}
#endif /* _DWA_SDK_H_ */
