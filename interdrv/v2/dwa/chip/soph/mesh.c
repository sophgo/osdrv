#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/sched/types.h>

#include <linux/cvi_comm_video.h>
#include <linux/cvi_comm_gdc.h>
#include <linux/dwa_uapi.h>
#include <linux/cvi_buffer.h>

#include "mesh.h"
#include "cvi_vip_dwa.h"
#include "vb.h"
#include "vbq.h"
#include "dwa_debug.h"
#include "dwa_sdk.h"

static s32 mesh_dwa_do_ldc_fisheye(struct cvi_dwa_vdev *wdev, enum dwa_usage usage, struct vb_s *vb_in
	, PIXEL_FORMAT_E enPixFormat, u64 mesh_addr, bool sync_io, void *pcbParam
	, u32 cbParamSize, MOD_ID_E enModId, ROTATION_E enRotation)
{
	struct dwa_handle_data data;
	struct dwa_task_attr *pstTask = NULL;
	VB_BLK blk;
	struct vb_s *vb_out;
	u32 buf_size;
	s32 ret;
	SIZE_S size_out;
	ROTATION_E enRotationOut;
	void *_pcbParam;
	u8 i;
	struct dwa_job *job;
	struct _dwa_cb_param *p_cb_param;

	pstTask = vmalloc(sizeof(struct dwa_task_attr));
	if (!pstTask) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "vmalloc failed\n");
		ret = CVI_ERR_DWA_NOMEM;
		goto DWA_FAIL_ALLOC;
	}

	if (enRotation == ROTATION_0 || enRotation == ROTATION_180) {
		size_out.u32Width = ALIGN(vb_in->buf.size.u32Width, DEFAULT_ALIGN / 2);
		size_out.u32Height = vb_in->buf.size.u32Height;
	} else {
		size_out.u32Width = ALIGN(vb_in->buf.size.u32Height, DEFAULT_ALIGN / 2);
		size_out.u32Height = vb_in->buf.size.u32Width;
	}

	// get buf for gdc output.
	buf_size = COMMON_GetPicBufferSize(size_out.u32Width, size_out.u32Height
		, PIXEL_FORMAT_YUV_PLANAR_420, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DWA_ALIGNMENT);
	blk = vb_get_block_with_id(wdev->VbPool, buf_size, CVI_ID_DWA);
	if (blk == VB_INVALID_HANDLE) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "get vb fail\n");
		vb_release_block((VB_BLK)vb_in);
		ret = CVI_ERR_DWA_NOBUF;
		goto DWA_FAIL_GET_VB;
	}

	atomic_long_fetch_and(~BIT(enModId), &vb_in->mod_ids);
	atomic_long_fetch_or(BIT(CVI_ID_DWA), &vb_in->mod_ids);

	vb_out = (struct vb_s *)blk;
	base_get_frame_info(enPixFormat, size_out, &vb_out->buf, vb_out->phy_addr, DWA_ALIGNMENT);

	CVI_TRACE_DWA(CVI_DBG_DEBUG, "DWA usage(%d) rot(%d) src phy-addr(0x%llx) dst phy-addr(0x%llx)\n"
	, usage, enRotation, vb_in->phy_addr, vb_out->phy_addr);

	dwa_begin_job(wdev, &data);

	memset(pstTask, 0, sizeof(*pstTask));
	pstTask->handle = data.handle;
	pstTask->enRotation = enRotationOut;

	// prepare the in/out image info of the gdc task.
	pstTask->stImgIn.stVFrame.enPixelFormat = enPixFormat;
	pstTask->stImgIn.stVFrame.u32Width = vb_in->buf.size.u32Width;
	pstTask->stImgIn.stVFrame.u32Height = vb_in->buf.size.u32Height;
	pstTask->stImgIn.stVFrame.u64PTS = vb_in->buf.u64PTS;
	for (i = 0; i < 3; ++i) {
		pstTask->stImgIn.stVFrame.u64PhyAddr[i] = vb_in->buf.phy_addr[i];
		pstTask->stImgIn.stVFrame.u32Length[i] = vb_in->buf.length[i];
		pstTask->stImgIn.stVFrame.u32Stride[i] = vb_in->buf.stride[i];
	}

	pstTask->stImgOut.stVFrame.enPixelFormat = enPixFormat;
	pstTask->stImgOut.stVFrame.u32Width = vb_out->buf.size.u32Width;
	pstTask->stImgOut.stVFrame.u32Height = vb_out->buf.size.u32Height;
	for (i = 0; i < 3; ++i) {
		pstTask->stImgOut.stVFrame.u64PhyAddr[i] = vb_out->buf.phy_addr[i];
		pstTask->stImgOut.stVFrame.u32Length[i] = vb_out->buf.length[i];
		pstTask->stImgOut.stVFrame.u32Stride[i] = vb_out->buf.stride[i];
	}

	pstTask->stImgOut.stVFrame.u64PTS = pstTask->stImgIn.stVFrame.u64PTS;
	pstTask->stImgOut.stVFrame.pPrivateData = vb_out;

	switch (enRotation) {
	default:
	case ROTATION_0:
		pstTask->stImgOut.stVFrame.s16OffsetTop = vb_in->buf.s16OffsetTop;
		pstTask->stImgOut.stVFrame.s16OffsetLeft = vb_in->buf.s16OffsetLeft;
		pstTask->stImgOut.stVFrame.s16OffsetBottom = vb_in->buf.s16OffsetBottom;
		pstTask->stImgOut.stVFrame.s16OffsetRight = vb_in->buf.s16OffsetRight;
		break;
	case ROTATION_90:
		pstTask->stImgOut.stVFrame.s16OffsetTop = vb_in->buf.s16OffsetLeft;
		pstTask->stImgOut.stVFrame.s16OffsetLeft = vb_in->buf.s16OffsetBottom;
		pstTask->stImgOut.stVFrame.s16OffsetBottom = vb_in->buf.s16OffsetRight;
		pstTask->stImgOut.stVFrame.s16OffsetRight = vb_in->buf.s16OffsetTop;
		vb_out->buf.frame_crop.start_x = vb_in->buf.frame_crop.start_y;
		vb_out->buf.frame_crop.end_x = vb_in->buf.frame_crop.end_y;
		vb_out->buf.frame_crop.start_y = vb_in->buf.size.u32Width - vb_in->buf.frame_crop.end_x;
		vb_out->buf.frame_crop.end_y = vb_in->buf.size.u32Width - vb_in->buf.frame_crop.start_x;
		break;
	case ROTATION_180:
		pstTask->stImgOut.stVFrame.s16OffsetTop = vb_in->buf.s16OffsetBottom;
		pstTask->stImgOut.stVFrame.s16OffsetLeft = vb_in->buf.s16OffsetRight;
		pstTask->stImgOut.stVFrame.s16OffsetBottom = vb_in->buf.s16OffsetTop;
		pstTask->stImgOut.stVFrame.s16OffsetRight = vb_in->buf.s16OffsetLeft;
		vb_out->buf.frame_crop.start_x = vb_in->buf.size.u32Width - vb_in->buf.frame_crop.end_x;
		vb_out->buf.frame_crop.end_x = vb_in->buf.size.u32Width - vb_in->buf.frame_crop.start_x;
		vb_out->buf.frame_crop.start_y = vb_in->buf.size.u32Height - vb_in->buf.frame_crop.end_y;
		vb_out->buf.frame_crop.end_y = vb_in->buf.size.u32Height - vb_in->buf.frame_crop.start_y;
		break;
	case ROTATION_270:
		pstTask->stImgOut.stVFrame.s16OffsetTop = vb_in->buf.s16OffsetRight;
		pstTask->stImgOut.stVFrame.s16OffsetLeft = vb_in->buf.s16OffsetTop;
		pstTask->stImgOut.stVFrame.s16OffsetBottom = vb_in->buf.s16OffsetLeft;
		pstTask->stImgOut.stVFrame.s16OffsetRight = vb_in->buf.s16OffsetBottom;
		vb_out->buf.frame_crop.start_x = vb_in->buf.size.u32Height - vb_in->buf.frame_crop.end_y;
		vb_out->buf.frame_crop.end_x = vb_in->buf.size.u32Height - vb_in->buf.frame_crop.start_y;
		vb_out->buf.frame_crop.start_y = vb_in->buf.frame_crop.start_x;
		vb_out->buf.frame_crop.end_y = vb_in->buf.frame_crop.end_x;
		break;
	}

	vb_out->buf.s16OffsetTop = pstTask->stImgOut.stVFrame.s16OffsetTop;
	vb_out->buf.s16OffsetBottom = pstTask->stImgOut.stVFrame.s16OffsetBottom;
	vb_out->buf.s16OffsetLeft = pstTask->stImgOut.stVFrame.s16OffsetLeft;
	vb_out->buf.s16OffsetRight = pstTask->stImgOut.stVFrame.s16OffsetRight;
	vb_out->buf.u64PTS = pstTask->stImgOut.stVFrame.u64PTS;
	vb_out->buf.frm_num = vb_in->buf.frm_num;
	vb_out->buf.motion_lv = vb_in->buf.motion_lv;
	memcpy(vb_out->buf.motion_table, vb_in->buf.motion_table, MO_TBL_SIZE);

	_pcbParam = vmalloc(cbParamSize);
	if (!_pcbParam) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "vmalloc failed, cbParamSize=%d\n", cbParamSize);
		ret = CVI_ERR_DWA_NOMEM;
		goto DWA_FAIL_EXIT;
	}

	memcpy(_pcbParam, pcbParam, cbParamSize);
	pstTask->au64privateData[0] = mesh_addr;
	pstTask->au64privateData[1] = (__u64)CVI_TRUE;
	pstTask->au64privateData[2] = (uintptr_t)_pcbParam;
	pstTask->reserved = CVI_DWA_MAGIC;

	if (usage == DWA_USAGE_LDC)
		dwa_add_ldc_task(wdev, pstTask);
	else
		dwa_add_cor_task(wdev, pstTask);

	job = (struct dwa_job *)(uintptr_t)data.handle;
	job->identity.syncIo = sync_io;
	job->identity.enModId = enModId;
	job->identity.u32ID = vb_in->buf.frm_num;
	p_cb_param = (struct _dwa_cb_param *)_pcbParam;
	snprintf(job->identity.Name, sizeof(job->identity.Name)
		, "dev_%d_chn_%d", p_cb_param->chn.s32DevId, p_cb_param->chn.s32ChnId);

	ret = dwa_end_job(wdev, data.handle);

	vfree(pstTask);
	return ret;

DWA_FAIL_EXIT:
	vb_release_block((VB_BLK)blk);
DWA_FAIL_GET_VB:
	vfree(pstTask);
DWA_FAIL_ALLOC:
	vb_release_block((VB_BLK)vb_in);

	return ret;
}

static s32 mesh_dwa_do_ldc_fisheye_gridinfo(struct cvi_dwa_vdev *wdev, enum dwa_usage usage
	, GRID_INFO_ATTR_S *pst_gridinfo_attr, struct vb_s *vb_in, PIXEL_FORMAT_E en_pixformat, u64 mesh_addr
	, bool sync_io, void *p_cb_param, u32 cb_param_size, MOD_ID_E en_mod_id)
{
	struct dwa_handle_data data;
	struct dwa_task_attr *pst_task = NULL;
	VB_BLK blk;
	struct vb_s *vb_out;
	u32 buf_size;
	s32 ret;
	SIZE_S size_out;
	void *_p_cb_param;
	u8 i;
	struct dwa_job *job;
	struct _dwa_cb_param *p_dwa_cb_param;

	pst_task = vmalloc(sizeof(*pst_task));
	if (!pst_task) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "vmalloc failed\n");
		ret = CVI_ERR_DWA_NOMEM;
		goto DWA_FAIL_ALLOC;
	}

	if (pst_gridinfo_attr->grid_out.u32Width && pst_gridinfo_attr->grid_out.u32Height) {
		size_out.u32Width = ALIGN(pst_gridinfo_attr->grid_out.u32Width, DWA_ALIGNMENT);
		size_out.u32Height = pst_gridinfo_attr->grid_out.u32Height;
	} else {
		size_out.u32Width = ALIGN(vb_in->buf.size.u32Height, DEFAULT_ALIGN / 2);
		size_out.u32Height = vb_in->buf.size.u32Width;
	}

	// get buf for gdc output.
	buf_size = COMMON_GetPicBufferSize(size_out.u32Width, size_out.u32Height
		, en_pixformat, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DWA_ALIGNMENT);
	blk = vb_get_block_with_id(wdev->VbPool, buf_size, CVI_ID_DWA);
	if (blk == VB_INVALID_HANDLE) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "get vb fail\n");
		vb_release_block((VB_BLK)vb_in);
		ret = CVI_ERR_DWA_NOBUF;
		goto DWA_FAIL_GET_VB;
	}

	atomic_long_fetch_and(~BIT(en_mod_id), &vb_in->mod_ids);
	atomic_long_fetch_or(BIT(CVI_ID_DWA), &vb_in->mod_ids);

	vb_out = (struct vb_s *)blk;
	base_get_frame_info(en_pixformat, size_out, &vb_out->buf, vb_out->phy_addr, DWA_ALIGNMENT);

	CVI_TRACE_DWA(CVI_DBG_DEBUG, "DWA usage(%d) src phy-addr(0x%llx) size(%d-%d) dst phy-addr(0x%llx) size(%d-%d)\n"
		, usage, vb_in->phy_addr, vb_in->buf.size.u32Width, vb_in->buf.size.u32Height
		, vb_out->phy_addr, size_out.u32Width, size_out.u32Height);

	dwa_begin_job(wdev, &data);

	memset(pst_task, 0, sizeof(*pst_task));
	pst_task->handle = data.handle;
	pst_task->enRotation = ROTATION_0;

	// prepare the in/out image info of the gdc task.
	pst_task->stImgIn.stVFrame.enPixelFormat = en_pixformat;
	pst_task->stImgIn.stVFrame.u32Width = vb_in->buf.size.u32Width;
	pst_task->stImgIn.stVFrame.u32Height = vb_in->buf.size.u32Height;
	pst_task->stImgIn.stVFrame.u64PTS = vb_in->buf.u64PTS;
	for (i = 0; i < 3; ++i) {
		pst_task->stImgIn.stVFrame.u64PhyAddr[i] = vb_in->buf.phy_addr[i];
		pst_task->stImgIn.stVFrame.u32Length[i] = vb_in->buf.length[i];
		pst_task->stImgIn.stVFrame.u32Stride[i] = vb_in->buf.stride[i];
	}

	pst_task->stImgOut.stVFrame.enPixelFormat = en_pixformat;
	pst_task->stImgOut.stVFrame.u32Width = vb_out->buf.size.u32Width;
	pst_task->stImgOut.stVFrame.u32Height = vb_out->buf.size.u32Height;
	for (i = 0; i < 3; ++i) {
		pst_task->stImgOut.stVFrame.u64PhyAddr[i] = vb_out->buf.phy_addr[i];
		pst_task->stImgOut.stVFrame.u32Length[i] = vb_out->buf.length[i];
		pst_task->stImgOut.stVFrame.u32Stride[i] = vb_out->buf.stride[i];
	}

	pst_task->stImgOut.stVFrame.u64PTS = pst_task->stImgIn.stVFrame.u64PTS;
	pst_task->stImgOut.stVFrame.pPrivateData = vb_out;

	switch (pst_task->enRotation) {
	default:
	case ROTATION_0:
		pst_task->stImgOut.stVFrame.s16OffsetTop = vb_in->buf.s16OffsetTop;
		pst_task->stImgOut.stVFrame.s16OffsetLeft = vb_in->buf.s16OffsetLeft;
		pst_task->stImgOut.stVFrame.s16OffsetBottom = vb_in->buf.s16OffsetBottom;
		pst_task->stImgOut.stVFrame.s16OffsetRight = vb_in->buf.s16OffsetRight;
		break;
	case ROTATION_90:
		pst_task->stImgOut.stVFrame.s16OffsetTop = vb_in->buf.s16OffsetLeft;
		pst_task->stImgOut.stVFrame.s16OffsetLeft = vb_in->buf.s16OffsetBottom;
		pst_task->stImgOut.stVFrame.s16OffsetBottom = vb_in->buf.s16OffsetRight;
		pst_task->stImgOut.stVFrame.s16OffsetRight = vb_in->buf.s16OffsetTop;
		vb_out->buf.frame_crop.start_x = vb_in->buf.frame_crop.start_y;
		vb_out->buf.frame_crop.end_x = vb_in->buf.frame_crop.end_y;
		vb_out->buf.frame_crop.start_y = vb_in->buf.size.u32Width - vb_in->buf.frame_crop.end_x;
		vb_out->buf.frame_crop.end_y = vb_in->buf.size.u32Width - vb_in->buf.frame_crop.start_x;
		break;
	case ROTATION_180:
		pst_task->stImgOut.stVFrame.s16OffsetTop = vb_in->buf.s16OffsetBottom;
		pst_task->stImgOut.stVFrame.s16OffsetLeft = vb_in->buf.s16OffsetRight;
		pst_task->stImgOut.stVFrame.s16OffsetBottom = vb_in->buf.s16OffsetTop;
		pst_task->stImgOut.stVFrame.s16OffsetRight = vb_in->buf.s16OffsetLeft;
		vb_out->buf.frame_crop.start_x = vb_in->buf.size.u32Width - vb_in->buf.frame_crop.end_x;
		vb_out->buf.frame_crop.end_x = vb_in->buf.size.u32Width - vb_in->buf.frame_crop.start_x;
		vb_out->buf.frame_crop.start_y = vb_in->buf.size.u32Height - vb_in->buf.frame_crop.end_y;
		vb_out->buf.frame_crop.end_y = vb_in->buf.size.u32Height - vb_in->buf.frame_crop.start_y;
		break;
	case ROTATION_270:
		pst_task->stImgOut.stVFrame.s16OffsetTop = vb_in->buf.s16OffsetRight;
		pst_task->stImgOut.stVFrame.s16OffsetLeft = vb_in->buf.s16OffsetTop;
		pst_task->stImgOut.stVFrame.s16OffsetBottom = vb_in->buf.s16OffsetLeft;
		pst_task->stImgOut.stVFrame.s16OffsetRight = vb_in->buf.s16OffsetBottom;
		vb_out->buf.frame_crop.start_x = vb_in->buf.size.u32Height - vb_in->buf.frame_crop.end_y;
		vb_out->buf.frame_crop.end_x = vb_in->buf.size.u32Height - vb_in->buf.frame_crop.start_y;
		vb_out->buf.frame_crop.start_y = vb_in->buf.frame_crop.start_x;
		vb_out->buf.frame_crop.end_y = vb_in->buf.frame_crop.end_x;
		break;
	}

	vb_out->buf.s16OffsetTop = pst_task->stImgOut.stVFrame.s16OffsetTop;
	vb_out->buf.s16OffsetBottom = pst_task->stImgOut.stVFrame.s16OffsetBottom;
	vb_out->buf.s16OffsetLeft = pst_task->stImgOut.stVFrame.s16OffsetLeft;
	vb_out->buf.s16OffsetRight = pst_task->stImgOut.stVFrame.s16OffsetRight;
	vb_out->buf.u64PTS = pst_task->stImgOut.stVFrame.u64PTS;
	vb_out->buf.frm_num = vb_in->buf.frm_num;
	vb_out->buf.motion_lv = vb_in->buf.motion_lv;
	memcpy(vb_out->buf.motion_table, vb_in->buf.motion_table, MO_TBL_SIZE);

	_p_cb_param = vmalloc(cb_param_size);
	if (!_p_cb_param) {
		CVI_TRACE_DWA(CVI_DBG_ERR, "vmalloc failed, cb_param_size=%d\n", cb_param_size);
		ret = CVI_ERR_DWA_NOMEM;
		goto DWA_FAIL_EXIT;
	}

	memcpy(_p_cb_param, p_cb_param, cb_param_size);
	pst_task->au64privateData[0] = mesh_addr;
	pst_task->au64privateData[1] = (__u64)CVI_TRUE;
	pst_task->au64privateData[2] = (uintptr_t)_p_cb_param;
	pst_task->reserved = CVI_DWA_MAGIC;

	if (usage == DWA_USAGE_LDC)
		dwa_add_ldc_task(wdev, pst_task);
	else
		dwa_add_cor_task(wdev, pst_task);

	job = (struct dwa_job *)(uintptr_t)data.handle;
	job->identity.syncIo = sync_io;
	job->identity.enModId = en_mod_id;
	job->identity.u32ID = vb_in->buf.frm_num;
	p_dwa_cb_param = (struct _dwa_cb_param *)_p_cb_param;
	snprintf(job->identity.Name, sizeof(job->identity.Name)
		, "dev_%d_chn_%d", p_dwa_cb_param->chn.s32DevId, p_dwa_cb_param->chn.s32ChnId);

	ret = dwa_end_job(wdev, data.handle);

	vfree(pst_task);
	return ret;

DWA_FAIL_EXIT:
	vb_release_block((VB_BLK)blk);
DWA_FAIL_GET_VB:
	vfree(pst_task);
DWA_FAIL_ALLOC:
	vb_release_block((VB_BLK)vb_in);

	return ret;
}

s32 mesh_dwa_do_op(struct cvi_dwa_vdev *wdev, enum dwa_usage usage
	, const void *pUsageParam, struct vb_s *vb_in, PIXEL_FORMAT_E enPixFormat
	, u64 mesh_addr, u8 sync_io, void *pcbParam, u32 cbParamSize
	, MOD_ID_E enModId, ROTATION_E enRotation)
{
	s32 ret = CVI_ERR_DWA_ILLEGAL_PARAM;
	LDC_ATTR_S stLDCAttr;
	FISHEYE_ATTR_S stFISHEYEAttr;

	CVI_TRACE_DWA(CVI_DBG_DEBUG, "DWA usage(%d) enRotation(%d), mesh-addr(0x%llx), cbParamSize(%d)\n",
			usage, enRotation, (unsigned long long)mesh_addr, cbParamSize);

	ret = dwa_check_null_ptr(pUsageParam);
	if (ret)
		return ret;

	switch (usage) {
	case DWA_USAGE_LDC:
		stLDCAttr = *(LDC_ATTR_S *)pUsageParam;
		if (stLDCAttr.stGridInfoAttr.Enable)
			ret = mesh_dwa_do_ldc_fisheye_gridinfo(wdev, usage, &stLDCAttr.stGridInfoAttr
				, vb_in, enPixFormat, mesh_addr
				, sync_io, pcbParam, cbParamSize, enModId);
		else
			ret = mesh_dwa_do_ldc_fisheye(wdev, usage, vb_in, enPixFormat, mesh_addr
				, sync_io, pcbParam, cbParamSize, enModId, enRotation);
		break;
	case DWA_USAGE_FISHEYE:
		stFISHEYEAttr = *(FISHEYE_ATTR_S *)pUsageParam;
		if (stFISHEYEAttr.stGridInfoAttr.Enable)
			ret = mesh_dwa_do_ldc_fisheye_gridinfo(wdev, usage, &stFISHEYEAttr.stGridInfoAttr
				, vb_in, enPixFormat, mesh_addr
				, sync_io, pcbParam, cbParamSize, enModId);
		else
			ret = mesh_dwa_do_ldc_fisheye(wdev, usage, vb_in, enPixFormat, mesh_addr
				, sync_io, pcbParam, cbParamSize, enModId, enRotation);
		break;
	default:
		CVI_TRACE_DWA(CVI_DBG_ERR, "DWA usage(%d) not support\n", usage);
		break;
	}

	return ret;
}
