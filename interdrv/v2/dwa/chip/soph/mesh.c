#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/sched/types.h>

#include <linux/comm_video.h>
#include <linux/comm_gdc.h>
#include <linux/dwa_uapi.h>
#include <linux/comm_buffer.h>

#include "mesh.h"
#include "dwa_core.h"
#include "vb.h"
#include "vbq.h"
#include "dwa_debug.h"
#include "dwa_sdk.h"

static int mesh_dwa_do_ldc_fisheye(struct dwa_vdev *wdev, enum dwa_usage usage, struct vb_s *vb_in
	, pixel_format_e pix_format, unsigned long long mesh_addr, bool sync_io, void *cb_param
	, unsigned int cb_param_size, mod_id_e mod_id, rotation_e rotation)
{
	struct dwa_handle_data data;
	struct dwa_task_attr *ptask = NULL;
	vb_blk blk;
	struct vb_s *vb_out;
	unsigned int buf_size;
	int ret;
	size_s size_out;
	rotation_e rotationOut;
	void *_cb_param;
	unsigned char i;
	struct dwa_job *job;
	struct _dwa_cb_param *p_cb_param;

	ptask = vmalloc(sizeof(struct dwa_task_attr));
	if (!ptask) {
		TRACE_DWA(DBG_ERR, "vmalloc failed\n");
		ret = ERR_DWA_NOMEM;
		goto DWA_FAIL_ALLOC;
	}

	if (rotation == ROTATION_0 || rotation == ROTATION_180) {
		size_out.width = ALIGN(vb_in->buf.size.width, DEFAULT_ALIGN / 2);
		size_out.height = vb_in->buf.size.height;
	} else {
		size_out.width = ALIGN(vb_in->buf.size.height, DEFAULT_ALIGN / 2);
		size_out.height = vb_in->buf.size.width;
	}

	// get buf for gdc output.
	buf_size = common_getpicbuffersize(size_out.width, size_out.height
		, PIXEL_FORMAT_YUV_PLANAR_420, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DWA_ALIGNMENT);
	blk = vb_get_block_with_id(wdev->vb_pool, buf_size, ID_DWA);
	if (blk == VB_INVALID_HANDLE) {
		TRACE_DWA(DBG_ERR, "get vb fail\n");
		vb_release_block((vb_blk)vb_in);
		ret = ERR_DWA_NOBUF;
		goto DWA_FAIL_GET_VB;
	}

	atomic_long_fetch_and(~BIT(mod_id), &vb_in->mod_ids);
	atomic_long_fetch_or(BIT(ID_DWA), &vb_in->mod_ids);

	vb_out = (struct vb_s *)blk;
	base_get_frame_info(pix_format, size_out, &vb_out->buf, vb_out->phy_addr, DWA_ALIGNMENT);

	TRACE_DWA(DBG_DEBUG, "DWA usage(%d) rot(%d) src phy-addr(0x%llx) dst phy-addr(0x%llx)\n"
	, usage, rotation, vb_in->phy_addr, vb_out->phy_addr);

	ret = dwa_begin_job(wdev, &data);
	if (ret) {
		TRACE_DWA(DBG_ERR, "dwa_begin_job fail, ret=%d, DWA usage(%d)\n", ret, usage);
		goto DWA_FAIL_EXIT;
	}

	memset(ptask, 0, sizeof(*ptask));
	ptask->handle = data.handle;
	ptask->rotation = rotationOut;

	// prepare the in/out image info of the gdc task.
	ptask->img_in.video_frame.pixel_format = pix_format;
	ptask->img_in.video_frame.width = vb_in->buf.size.width;
	ptask->img_in.video_frame.height = vb_in->buf.size.height;
	ptask->img_in.video_frame.pts = vb_in->buf.pts;
	for (i = 0; i < 3; ++i) {
		ptask->img_in.video_frame.phyaddr[i] = vb_in->buf.phy_addr[i];
		ptask->img_in.video_frame.length[i] = vb_in->buf.length[i];
		ptask->img_in.video_frame.stride[i] = vb_in->buf.stride[i];
	}

	ptask->img_out.video_frame.pixel_format = pix_format;
	ptask->img_out.video_frame.width = vb_out->buf.size.width;
	ptask->img_out.video_frame.height = vb_out->buf.size.height;
	for (i = 0; i < 3; ++i) {
		ptask->img_out.video_frame.phyaddr[i] = vb_out->buf.phy_addr[i];
		ptask->img_out.video_frame.length[i] = vb_out->buf.length[i];
		ptask->img_out.video_frame.stride[i] = vb_out->buf.stride[i];
	}

	ptask->img_out.video_frame.pts = ptask->img_in.video_frame.pts;
	ptask->img_out.video_frame.private_data = vb_out;

	switch (rotation) {
	default:
	case ROTATION_0:
		ptask->img_out.video_frame.offset_top = vb_in->buf.offset_top;
		ptask->img_out.video_frame.offset_left = vb_in->buf.offset_left;
		ptask->img_out.video_frame.offset_bottom = vb_in->buf.offset_bottom;
		ptask->img_out.video_frame.offset_right = vb_in->buf.offset_right;
		break;
	case ROTATION_90:
		ptask->img_out.video_frame.offset_top = vb_in->buf.offset_left;
		ptask->img_out.video_frame.offset_left = vb_in->buf.offset_bottom;
		ptask->img_out.video_frame.offset_bottom = vb_in->buf.offset_right;
		ptask->img_out.video_frame.offset_right = vb_in->buf.offset_top;
		vb_out->buf.frame_crop.start_x = vb_in->buf.frame_crop.start_y;
		vb_out->buf.frame_crop.end_x = vb_in->buf.frame_crop.end_y;
		vb_out->buf.frame_crop.start_y = vb_in->buf.size.width - vb_in->buf.frame_crop.end_x;
		vb_out->buf.frame_crop.end_y = vb_in->buf.size.width - vb_in->buf.frame_crop.start_x;
		break;
	case ROTATION_180:
		ptask->img_out.video_frame.offset_top = vb_in->buf.offset_bottom;
		ptask->img_out.video_frame.offset_left = vb_in->buf.offset_right;
		ptask->img_out.video_frame.offset_bottom = vb_in->buf.offset_top;
		ptask->img_out.video_frame.offset_right = vb_in->buf.offset_left;
		vb_out->buf.frame_crop.start_x = vb_in->buf.size.width - vb_in->buf.frame_crop.end_x;
		vb_out->buf.frame_crop.end_x = vb_in->buf.size.width - vb_in->buf.frame_crop.start_x;
		vb_out->buf.frame_crop.start_y = vb_in->buf.size.height - vb_in->buf.frame_crop.end_y;
		vb_out->buf.frame_crop.end_y = vb_in->buf.size.height - vb_in->buf.frame_crop.start_y;
		break;
	case ROTATION_270:
		ptask->img_out.video_frame.offset_top = vb_in->buf.offset_right;
		ptask->img_out.video_frame.offset_left = vb_in->buf.offset_top;
		ptask->img_out.video_frame.offset_bottom = vb_in->buf.offset_left;
		ptask->img_out.video_frame.offset_right = vb_in->buf.offset_bottom;
		vb_out->buf.frame_crop.start_x = vb_in->buf.size.height - vb_in->buf.frame_crop.end_y;
		vb_out->buf.frame_crop.end_x = vb_in->buf.size.height - vb_in->buf.frame_crop.start_y;
		vb_out->buf.frame_crop.start_y = vb_in->buf.frame_crop.start_x;
		vb_out->buf.frame_crop.end_y = vb_in->buf.frame_crop.end_x;
		break;
	}

	vb_out->buf.offset_top = ptask->img_out.video_frame.offset_top;
	vb_out->buf.offset_bottom = ptask->img_out.video_frame.offset_bottom;
	vb_out->buf.offset_left = ptask->img_out.video_frame.offset_left;
	vb_out->buf.offset_right = ptask->img_out.video_frame.offset_right;
	vb_out->buf.pts = ptask->img_out.video_frame.pts;
	vb_out->buf.frm_num = vb_in->buf.frm_num;
	vb_out->buf.motion_lv = vb_in->buf.motion_lv;
	memcpy(vb_out->buf.motion_table, vb_in->buf.motion_table, MO_TBL_SIZE);

	_cb_param = vmalloc(cb_param_size);
	if (!_cb_param) {
		TRACE_DWA(DBG_ERR, "vmalloc failed, cb_param_size=%d\n", cb_param_size);
		ret = ERR_DWA_NOMEM;
		goto DWA_FAIL_EXIT;
	}

	memcpy(_cb_param, cb_param, cb_param_size);
	ptask->private_data[0] = mesh_addr;
	ptask->private_data[1] = (__u64)true;
	ptask->private_data[2] = (uintptr_t)_cb_param;
	ptask->reserved = DWA_MAGIC;

	if (usage == DWA_USAGE_LDC)
		ret = dwa_add_ldc_task(wdev, ptask);
	else
		ret = dwa_add_cor_task(wdev, ptask);
	if (ret) {
		TRACE_DWA(DBG_ERR, "dwa_add_xxx_tsk fail, ret=%d, DWA usage(%d)\n", ret, usage);
		goto DWA_FAIL_EXIT;
	}

	job = (struct dwa_job *)(uintptr_t)data.handle;
	job->identity.sync_io = sync_io;
	job->identity.mod_id = mod_id;
	job->identity.id = vb_in->buf.frm_num;
	p_cb_param = (struct _dwa_cb_param *)_cb_param;
	snprintf(job->identity.name, sizeof(job->identity.name)
		, "dev_%d_chn_%d", p_cb_param->chn.dev_id, p_cb_param->chn.chn_id);

	ret = dwa_end_job(wdev, data.handle);
	if (ret) {
		TRACE_DWA(DBG_ERR, "dwa_end_job fail, ret=%d, DWA usage(%d)\n", ret, usage);
		goto DWA_FAIL_EXIT;
	}

	vfree(ptask);
	return ret;

DWA_FAIL_EXIT:
	vb_release_block((vb_blk)blk);
DWA_FAIL_GET_VB:
	vfree(ptask);
DWA_FAIL_ALLOC:
	vb_release_block((vb_blk)vb_in);

	return ret;
}

static int mesh_dwa_do_ldc_fisheye_gridinfo(struct dwa_vdev *wdev, enum dwa_usage usage
	, grid_info_attr_s *pst_gridinfo_attr, struct vb_s *vb_in, pixel_format_e en_pixformat, unsigned long long mesh_addr
	, bool sync_io, void *p_cb_param, unsigned int cb_param_size, mod_id_e en_mod_id)
{
	struct dwa_handle_data data;
	struct dwa_task_attr *pst_task = NULL;
	vb_blk blk;
	struct vb_s *vb_out;
	unsigned int buf_size;
	int ret;
	size_s size_out;
	void *_p_cb_param;
	unsigned char i;
	struct dwa_job *job;
	struct _dwa_cb_param *p_dwa_cb_param;

	pst_task = vmalloc(sizeof(*pst_task));
	if (!pst_task) {
		TRACE_DWA(DBG_ERR, "vmalloc failed\n");
		ret = ERR_DWA_NOMEM;
		goto DWA_FAIL_ALLOC;
	}

	if (pst_gridinfo_attr->grid_out.width && pst_gridinfo_attr->grid_out.height) {
		size_out.width = ALIGN(pst_gridinfo_attr->grid_out.width, DWA_ALIGNMENT);
		size_out.height = pst_gridinfo_attr->grid_out.height;
	} else {
		size_out.width = ALIGN(vb_in->buf.size.height, DEFAULT_ALIGN / 2);
		size_out.height = vb_in->buf.size.width;
	}

	// get buf for gdc output.
	buf_size = common_getpicbuffersize(size_out.width, size_out.height
		, en_pixformat, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DWA_ALIGNMENT);
	blk = vb_get_block_with_id(wdev->vb_pool, buf_size, ID_DWA);
	if (blk == VB_INVALID_HANDLE) {
		TRACE_DWA(DBG_ERR, "get vb fail\n");
		vb_release_block((vb_blk)vb_in);
		ret = ERR_DWA_NOBUF;
		goto DWA_FAIL_GET_VB;
	}

	atomic_long_fetch_and(~BIT(en_mod_id), &vb_in->mod_ids);
	atomic_long_fetch_or(BIT(ID_DWA), &vb_in->mod_ids);

	vb_out = (struct vb_s *)blk;
	base_get_frame_info(en_pixformat, size_out, &vb_out->buf, vb_out->phy_addr, DWA_ALIGNMENT);

	TRACE_DWA(DBG_DEBUG, "DWA usage(%d) src phy-addr(0x%llx) size(%d-%d) dst phy-addr(0x%llx) size(%d-%d)\n"
		, usage, vb_in->phy_addr, vb_in->buf.size.width, vb_in->buf.size.height
		, vb_out->phy_addr, size_out.width, size_out.height);

	ret = dwa_begin_job(wdev, &data);
	if (ret) {
		TRACE_DWA(DBG_ERR, "dwa_begin_job fail, ret=%d, DWA usage(%d)\n", ret, usage);
		goto DWA_FAIL_EXIT;
	}

	memset(pst_task, 0, sizeof(*pst_task));
	pst_task->handle = data.handle;
	pst_task->rotation = ROTATION_0;

	// prepare the in/out image info of the gdc task.
	pst_task->img_in.video_frame.pixel_format = en_pixformat;
	pst_task->img_in.video_frame.width = vb_in->buf.size.width;
	pst_task->img_in.video_frame.height = vb_in->buf.size.height;
	pst_task->img_in.video_frame.pts = vb_in->buf.pts;
	for (i = 0; i < 3; ++i) {
		pst_task->img_in.video_frame.phyaddr[i] = vb_in->buf.phy_addr[i];
		pst_task->img_in.video_frame.length[i] = vb_in->buf.length[i];
		pst_task->img_in.video_frame.stride[i] = vb_in->buf.stride[i];
	}

	pst_task->img_out.video_frame.pixel_format = en_pixformat;
	pst_task->img_out.video_frame.width = vb_out->buf.size.width;
	pst_task->img_out.video_frame.height = vb_out->buf.size.height;
	for (i = 0; i < 3; ++i) {
		pst_task->img_out.video_frame.phyaddr[i] = vb_out->buf.phy_addr[i];
		pst_task->img_out.video_frame.length[i] = vb_out->buf.length[i];
		pst_task->img_out.video_frame.stride[i] = vb_out->buf.stride[i];
	}

	pst_task->img_out.video_frame.pts = pst_task->img_in.video_frame.pts;
	pst_task->img_out.video_frame.private_data = vb_out;

	switch (pst_task->rotation) {
	default:
	case ROTATION_0:
		pst_task->img_out.video_frame.offset_top = vb_in->buf.offset_top;
		pst_task->img_out.video_frame.offset_left = vb_in->buf.offset_left;
		pst_task->img_out.video_frame.offset_bottom = vb_in->buf.offset_bottom;
		pst_task->img_out.video_frame.offset_right = vb_in->buf.offset_right;
		break;
	case ROTATION_90:
		pst_task->img_out.video_frame.offset_top = vb_in->buf.offset_left;
		pst_task->img_out.video_frame.offset_left = vb_in->buf.offset_bottom;
		pst_task->img_out.video_frame.offset_bottom = vb_in->buf.offset_right;
		pst_task->img_out.video_frame.offset_right = vb_in->buf.offset_top;
		vb_out->buf.frame_crop.start_x = vb_in->buf.frame_crop.start_y;
		vb_out->buf.frame_crop.end_x = vb_in->buf.frame_crop.end_y;
		vb_out->buf.frame_crop.start_y = vb_in->buf.size.width - vb_in->buf.frame_crop.end_x;
		vb_out->buf.frame_crop.end_y = vb_in->buf.size.width - vb_in->buf.frame_crop.start_x;
		break;
	case ROTATION_180:
		pst_task->img_out.video_frame.offset_top = vb_in->buf.offset_bottom;
		pst_task->img_out.video_frame.offset_left = vb_in->buf.offset_right;
		pst_task->img_out.video_frame.offset_bottom = vb_in->buf.offset_top;
		pst_task->img_out.video_frame.offset_right = vb_in->buf.offset_left;
		vb_out->buf.frame_crop.start_x = vb_in->buf.size.width - vb_in->buf.frame_crop.end_x;
		vb_out->buf.frame_crop.end_x = vb_in->buf.size.width - vb_in->buf.frame_crop.start_x;
		vb_out->buf.frame_crop.start_y = vb_in->buf.size.height - vb_in->buf.frame_crop.end_y;
		vb_out->buf.frame_crop.end_y = vb_in->buf.size.height - vb_in->buf.frame_crop.start_y;
		break;
	case ROTATION_270:
		pst_task->img_out.video_frame.offset_top = vb_in->buf.offset_right;
		pst_task->img_out.video_frame.offset_left = vb_in->buf.offset_top;
		pst_task->img_out.video_frame.offset_bottom = vb_in->buf.offset_left;
		pst_task->img_out.video_frame.offset_right = vb_in->buf.offset_bottom;
		vb_out->buf.frame_crop.start_x = vb_in->buf.size.height - vb_in->buf.frame_crop.end_y;
		vb_out->buf.frame_crop.end_x = vb_in->buf.size.height - vb_in->buf.frame_crop.start_y;
		vb_out->buf.frame_crop.start_y = vb_in->buf.frame_crop.start_x;
		vb_out->buf.frame_crop.end_y = vb_in->buf.frame_crop.end_x;
		break;
	}

	vb_out->buf.offset_top = pst_task->img_out.video_frame.offset_top;
	vb_out->buf.offset_bottom = pst_task->img_out.video_frame.offset_bottom;
	vb_out->buf.offset_left = pst_task->img_out.video_frame.offset_left;
	vb_out->buf.offset_right = pst_task->img_out.video_frame.offset_right;
	vb_out->buf.pts = pst_task->img_out.video_frame.pts;
	vb_out->buf.frm_num = vb_in->buf.frm_num;
	vb_out->buf.motion_lv = vb_in->buf.motion_lv;
	memcpy(vb_out->buf.motion_table, vb_in->buf.motion_table, MO_TBL_SIZE);

	_p_cb_param = vmalloc(cb_param_size);
	if (!_p_cb_param) {
		TRACE_DWA(DBG_ERR, "vmalloc failed, cb_param_size=%d\n", cb_param_size);
		ret = ERR_DWA_NOMEM;
		goto DWA_FAIL_EXIT;
	}

	memcpy(_p_cb_param, p_cb_param, cb_param_size);
	pst_task->private_data[0] = mesh_addr;
	pst_task->private_data[1] = (__u64)true;
	pst_task->private_data[2] = (uintptr_t)_p_cb_param;
	pst_task->reserved = DWA_MAGIC;

	if (usage == DWA_USAGE_LDC)
		ret = dwa_add_ldc_task(wdev, pst_task);
	else
		ret = dwa_add_cor_task(wdev, pst_task);
	if (ret) {
		TRACE_DWA(DBG_ERR, "dwa_add_xxx_tsk fail, ret=%d, DWA usage(%d)\n", ret, usage);
		goto DWA_FAIL_EXIT;
	}

	job = (struct dwa_job *)(uintptr_t)data.handle;
	job->identity.sync_io = sync_io;
	job->identity.mod_id = en_mod_id;
	job->identity.id = vb_in->buf.frm_num;
	p_dwa_cb_param = (struct _dwa_cb_param *)_p_cb_param;
	snprintf(job->identity.name, sizeof(job->identity.name)
		, "dev_%d_chn_%d", p_dwa_cb_param->chn.dev_id, p_dwa_cb_param->chn.chn_id);

	ret = dwa_end_job(wdev, data.handle);
	if (ret) {
		TRACE_DWA(DBG_ERR, "dwa_end_job fail, ret=%d, DWA usage(%d)\n", ret, usage);
		goto DWA_FAIL_EXIT;
	}

	vfree(pst_task);
	return ret;

DWA_FAIL_EXIT:
	vb_release_block((vb_blk)blk);
DWA_FAIL_GET_VB:
	vfree(pst_task);
DWA_FAIL_ALLOC:
	vb_release_block((vb_blk)vb_in);

	return ret;
}

int mesh_dwa_do_op(struct dwa_vdev *wdev, enum dwa_usage usage
	, const void *usage_param, struct vb_s *vb_in, pixel_format_e pix_format
	, unsigned long long mesh_addr, unsigned char sync_io, void *cb_param, unsigned int cb_param_size
	, mod_id_e mod_id, rotation_e rotation)
{
	int ret = ERR_DWA_ILLEGAL_PARAM;
	ldc_attr_s ldc_attr;
	fisheye_attr_s stFISHEYEAttr;

	TRACE_DWA(DBG_DEBUG, "DWA usage(%d) rotation(%d), mesh-addr(0x%llx), cb_param_size(%d)\n",
			usage, rotation, (unsigned long long)mesh_addr, cb_param_size);

	ret = dwa_check_null_ptr(usage_param);
	if (ret)
		return ret;

	switch (usage) {
	case DWA_USAGE_LDC:
		ldc_attr = *(ldc_attr_s *)usage_param;
		if (ldc_attr.grid_info_attr.enable)
			ret = mesh_dwa_do_ldc_fisheye_gridinfo(wdev, usage, &ldc_attr.grid_info_attr
				, vb_in, pix_format, mesh_addr
				, sync_io, cb_param, cb_param_size, mod_id);
		else
			ret = mesh_dwa_do_ldc_fisheye(wdev, usage, vb_in, pix_format, mesh_addr
				, sync_io, cb_param, cb_param_size, mod_id, rotation);
		break;
	case DWA_USAGE_FISHEYE:
		stFISHEYEAttr = *(fisheye_attr_s *)usage_param;
		if (stFISHEYEAttr.grid_info_attr.enable)
			ret = mesh_dwa_do_ldc_fisheye_gridinfo(wdev, usage, &stFISHEYEAttr.grid_info_attr
				, vb_in, pix_format, mesh_addr
				, sync_io, cb_param, cb_param_size, mod_id);
		else
			ret = mesh_dwa_do_ldc_fisheye(wdev, usage, vb_in, pix_format, mesh_addr
				, sync_io, cb_param, cb_param_size, mod_id, rotation);
		break;
	default:
		TRACE_DWA(DBG_ERR, "DWA usage(%d) not support\n", usage);
		break;
	}

	return ret;
}
