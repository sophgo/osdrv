#include "osal_types.h"

#include "comm_video.h"
#include "comm_gdc.h"
#include "ldc_uapi.h"
#include "ldc_debug.h"
#include "ldc_sdk.h"
#include "ldc.h"
#include "mesh.h"
#include "vbq.h"
#include "comm_buffer.h"

#define TILESIZE 64 // HW: data Tile Size
#define HW_MESH_SIZE 8

#define MESH_NUM_ATILE (TILESIZE / HW_MESH_SIZE) // how many mesh in A TILE

typedef struct COORD2D_INT_HW {
	unsigned char xcor[3]; // s13.10, 24bit
} __attribute__((packed)) COORD2D_INT_HW;

void mesh_gen_get_1st_size(size_s in_size, unsigned int *mesh_1st_size)
{
	unsigned int ori_src_width, ori_src_height, src_width_s1, src_height_s1;
	unsigned int dst_height_s1, dst_width_s1, num_tilex_s1, num_tiley_s1;

	if (!mesh_1st_size)
		return;

	ori_src_width = in_size.width;
	ori_src_height = in_size.height;

	// In LDC Processing, width & height  aligned to TILESIZE **
	src_width_s1 = ((ori_src_width + TILESIZE - 1) / TILESIZE) * TILESIZE;
	src_height_s1 = ((ori_src_height + TILESIZE - 1) / TILESIZE) * TILESIZE;

	// modify frame size
	dst_height_s1 = src_height_s1;
	dst_width_s1 = src_width_s1;
	num_tilex_s1 = dst_width_s1 / TILESIZE;
	num_tiley_s1 = dst_height_s1 / TILESIZE;

	*mesh_1st_size = sizeof(struct COORD2D_INT_HW) * MESH_NUM_ATILE *
			 MESH_NUM_ATILE * num_tilex_s1 * num_tiley_s1 *
			 4; // 4 = 4 knots in a mesh
}

static int init_ldc_param(const struct vb_s *vb_in, struct vb_s *vb_out
	, struct gdc_task_attr *task, pixel_format_e pix_format, unsigned long long mesh_addr
	, unsigned char isLatask, void *cb_param, unsigned int cb_param_size, rotation_e rotation)
{
	int i;
	void *_cb_param;
	short offset_top;
	short offset_bottom;
	short offset_left;
	short offset_right;
	unsigned int width;
	unsigned int height;

	if(vb_in->buf.size.width % DEFAULT_ALIGN) {
		width = ALIGN(vb_in->buf.size.width, DEFAULT_ALIGN);
		offset_left = 0;
		offset_right = ALIGN(vb_in->buf.size.width, DEFAULT_ALIGN) - vb_in->buf.size.width;
	} else {
		width = vb_in->buf.size.width;
		offset_left = vb_in->buf.offset_left;
		offset_right = vb_in->buf.offset_right;
	}

	if(vb_in->buf.size.height % DEFAULT_ALIGN) {
		height = ALIGN(vb_in->buf.size.height, DEFAULT_ALIGN);
		offset_top = 0;
		offset_bottom = ALIGN(vb_in->buf.size.height, DEFAULT_ALIGN) - vb_in->buf.size.height;
	} else {
		height = vb_in->buf.size.height;
		offset_top = vb_in->buf.offset_top;
		offset_bottom = vb_in->buf.offset_bottom;
	}

	osal_memset(task, 0, sizeof(*task));
	// prepare the in/out image info of the gdc task.
	task->img_in.video_frame.pixel_format = pix_format;
	task->img_in.video_frame.width = width - offset_left - offset_right;
	task->img_in.video_frame.height = height - offset_top - offset_bottom;
	task->img_in.video_frame.pts = vb_in->buf.pts;
	for (i = 0; i < 3; ++i) {
		task->img_in.video_frame.phyaddr[i] = vb_in->buf.phy_addr[i];
		task->img_in.video_frame.length[i] = vb_in->buf.length[i];
		task->img_in.video_frame.stride[i] = vb_in->buf.stride[i];
	}

	task->img_out.video_frame.pixel_format = pix_format;
	task->img_out.video_frame.width = vb_out->buf.size.width;
	task->img_out.video_frame.height = vb_out->buf.size.height;
	for (i = 0; i < 3; ++i) {
		task->img_out.video_frame.phyaddr[i] =
			vb_out->buf.phy_addr[i];
		task->img_out.video_frame.length[i] = vb_out->buf.length[i];
		task->img_out.video_frame.stride[i] = vb_out->buf.stride[i];
	}

	task->img_out.video_frame.pts = task->img_in.video_frame.pts;
	task->img_out.video_frame.private_data = vb_out;

	switch (rotation) {
	default:
	case ROTATION_0:
		task->img_out.video_frame.offset_top = offset_top;
		task->img_out.video_frame.offset_left = offset_left;
		task->img_out.video_frame.offset_bottom = offset_bottom;
		task->img_out.video_frame.offset_right = offset_right;
		break;
	case ROTATION_90:
		task->img_out.video_frame.offset_top = offset_left;
		task->img_out.video_frame.offset_left = offset_bottom;
		task->img_out.video_frame.offset_bottom = offset_right;
		task->img_out.video_frame.offset_right = offset_top;
		break;
	case ROTATION_180:
		task->img_out.video_frame.offset_top = offset_bottom;
		task->img_out.video_frame.offset_left = offset_right;
		task->img_out.video_frame.offset_bottom = offset_top;
		task->img_out.video_frame.offset_right = offset_left;
		break;
	case ROTATION_270:
		task->img_out.video_frame.offset_top = offset_right;
		task->img_out.video_frame.offset_left = offset_top;
		task->img_out.video_frame.offset_bottom = offset_left;
		task->img_out.video_frame.offset_right = offset_bottom;
		break;
	}

	vb_out->buf.offset_top = task->img_out.video_frame.offset_top;
	vb_out->buf.offset_bottom = task->img_out.video_frame.offset_bottom;
	vb_out->buf.offset_left = task->img_out.video_frame.offset_left;
	vb_out->buf.offset_right = task->img_out.video_frame.offset_right;
	vb_out->buf.pts = task->img_out.video_frame.pts;
	vb_out->buf.frm_num = vb_in->buf.frm_num;
	vb_out->buf.motion_lv = vb_in->buf.motion_lv;
	osal_memcpy(vb_out->buf.motion_table, vb_in->buf.motion_table,
	       MO_TBL_SIZE);

	_cb_param = osal_vmalloc(cb_param_size);
	if (!_cb_param) {
		TRACE_LDC(DBG_ERR, "vmalloc failed, cb_param_size=%d\n", cb_param_size);
		return -1;
	}

	osal_memcpy(_cb_param, cb_param, cb_param_size);
	task->private_data[0] = mesh_addr;
	task->private_data[1] = isLatask;
	task->private_data[2] = (uintptr_t)_cb_param;
	task->reserved = GDC_MAGIC;

	return 0;
}

static int mesh_gdc_do_rot(struct ldc_ctx *ctx, struct vb_s *vb_in
	, pixel_format_e pix_format, unsigned long long mesh_addr, bool sync_io, void *cb_param
	, unsigned int cb_param_size, mod_id_e mod_id, rotation_e rotation)
{
	struct gdc_handle_data data;
	struct ldc_job *job;
	struct gdc_task_attr *ptask = NULL;
	vb_blk blk;
	struct vb_s *vb_out;
	unsigned int buf_size;
	int ret;
	size_s size_out;
	struct _gdc_cb_param *p_cb_param;
	mmf_chn_s *chn;

	if (rotation == ROTATION_0 || rotation == ROTATION_180) {
		size_out.width = ALIGN(vb_in->buf.size.width, DEFAULT_ALIGN);
		size_out.height = ALIGN(vb_in->buf.size.height, DEFAULT_ALIGN);
	} else {
		size_out.width = ALIGN(vb_in->buf.size.height, DEFAULT_ALIGN);
		size_out.height = ALIGN(vb_in->buf.size.width, DEFAULT_ALIGN);
	}

	ptask = osal_vmalloc(sizeof(*ptask));
	if (!ptask) {
		TRACE_LDC(DBG_ERR, "vmalloc failed\n");
		ret = ERR_GDC_NOMEM;
		goto FAIL_ALLOC;
	}

	// get buf for gdc output.
	buf_size = common_getpicbuffersize(size_out.width, size_out.height
		, PIXEL_FORMAT_NV21, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
	p_cb_param = (struct _gdc_cb_param *)cb_param;
	chn = (mmf_chn_s *)(&(p_cb_param->chn));
	blk = vb_get_block_with_id(ctx->vb_pool[chn->mod_id == ID_VI ? 0 : chn->mod_id == ID_VPSS ? 1 : 2][chn->dev_id][chn->chn_id], buf_size, ID_GDC);
	if (blk == VB_INVALID_HANDLE) {
		TRACE_LDC(DBG_ERR, "get vb fail\n");
		ret = ERR_GDC_NOBUF;
		goto FAIL_GET_VB;
	}

	osal_atomic_fetch_and(~BIT(mod_id), &vb_in->mod_ids);
	osal_atomic_fetch_or(BIT(ID_GDC), &vb_in->mod_ids);

	vb_out = (struct vb_s *)(uintptr_t)blk;
	base_get_frame_info(pix_format, size_out, &vb_out->buf, vb_out->phy_addr, DEFAULT_ALIGN);

	TRACE_LDC(DBG_DEBUG, "GDC usage(%d) rot(%d) src phy-addr(0x%llx) dst phy-addr(0x%llx)\n"
		, GDC_USAGE_ROTATION, rotation, vb_in->phy_addr, vb_out->phy_addr);

	ret = init_ldc_param(vb_in, vb_out, ptask, pix_format, mesh_addr
		, true, cb_param, cb_param_size, rotation);
	if (ret) {
		TRACE_LDC(DBG_ERR, "init_ldc_param fail\n");
		goto FAIL_EXIT;
	}

	ret = ldc_begin_job(ctx, &data);
	if (ret) {
		TRACE_LDC(DBG_ERR, "ldc_begin_job fail, ret=%d, GDC usage(%d)\n", ret, GDC_USAGE_ROTATION);
		goto FAIL_EXIT;
	}
	ptask->handle = data.handle;
	ptask->rotation = rotation;
	ret = ldc_add_rotation_task(ctx, ptask);
	if (ret) {
		TRACE_LDC(DBG_ERR, "ldc_add_rotation_task fail, ret=%d, GDC usage(%d)\n", ret, GDC_USAGE_ROTATION);
		goto FAIL_EXIT;
	}

	job = (struct ldc_job *)(uintptr_t)data.handle;
	job->identity.sync_io = sync_io;
	job->identity.mod_id = mod_id;
	job->identity.id = vb_in->buf.frm_num;
	snprintf(job->identity.name, sizeof(job->identity.name)
		, "dev_%d_chn_%d", p_cb_param->chn.dev_id, p_cb_param->chn.chn_id);

	ret = ldc_end_job(ctx, data.handle);
	if (ret) {
		TRACE_LDC(DBG_ERR, "ldc_end_job fail, ret=%d, GDC usage(%d)\n", ret, GDC_USAGE_ROTATION);
		goto FAIL_EXIT;
	}

	osal_vfree(ptask);
	return ret;

FAIL_EXIT:
	vb_release_block((vb_blk)(uintptr_t)blk);
FAIL_GET_VB:
	osal_vfree(ptask);
FAIL_ALLOC:
	vb_release_block((vb_blk)(uintptr_t)vb_in);

	return ret;

}

static int mesh_gdc_do_ldc(struct ldc_ctx *ctx, const void *usage_param, struct vb_s *vb_in
	, pixel_format_e pix_format, unsigned long long mesh_addr, bool sync_io, void *cb_param
	, unsigned int cb_param_size, mod_id_e mod_id, rotation_e rotation)
{
	struct gdc_handle_data data;
	struct gdc_task_attr *ptask[2] = {NULL, NULL};
	vb_blk blk;
	struct vb_s *vb_out[2];
	unsigned int buf_size;
	int ret;
	size_s size_out[2];
	rotation_e rotation_out[2];
	unsigned int mesh_1st_size;
	struct ldc_job *job;
	struct _gdc_cb_param *p_cb_param;
	ldc_attr_s ldc_attr;
	mmf_chn_s *chn;
	ptask[0] = osal_vmalloc(sizeof(struct gdc_task_attr));
	ptask[1] = osal_vmalloc(sizeof(struct gdc_task_attr));
	if (!ptask[0] || !ptask[1]) {
		TRACE_LDC(DBG_ERR, "vmalloc failed\n");
		ret = ERR_GDC_NOMEM;
		goto FAIL_ALLOC;
	}

	// Rotate 90/270 for 1st job
	size_out[0].width = ALIGN(vb_in->buf.size.height, DEFAULT_ALIGN);
	size_out[0].height = ALIGN(vb_in->buf.size.width, DEFAULT_ALIGN);

	if (rotation == ROTATION_0 || rotation == ROTATION_180) {
		size_out[1].width = ALIGN(vb_in->buf.size.width, DEFAULT_ALIGN);
		size_out[1].height = ALIGN(vb_in->buf.size.height, DEFAULT_ALIGN);
	} else {
		size_out[1].width = ALIGN(vb_in->buf.size.height, DEFAULT_ALIGN);
		size_out[1].height = ALIGN(vb_in->buf.size.width, DEFAULT_ALIGN);
	}

	// get buf for gdc output.
	buf_size = common_getpicbuffersize(size_out[0].width, size_out[0].height
		, PIXEL_FORMAT_NV21, DATA_BITWIDTH_8, COMPRESS_MODE_NONE, DEFAULT_ALIGN);
	p_cb_param = (struct _gdc_cb_param *)cb_param;
	chn = (mmf_chn_s *)(&(p_cb_param->chn));
	blk = vb_get_block_with_id(ctx->vb_pool[chn->mod_id == ID_VI ? 0 : chn->mod_id == ID_VPSS ? 1 : 2][chn->dev_id][chn->chn_id], buf_size, ID_GDC);
	if (blk == VB_INVALID_HANDLE) {
		TRACE_LDC(DBG_ERR, "get vb fail\n");
		ret = ERR_GDC_NOBUF;
		goto FAIL_GET_VB;
	}

	osal_atomic_fetch_and(~BIT(mod_id), &vb_in->mod_ids);
	osal_atomic_fetch_or(BIT(ID_GDC), &vb_in->mod_ids);

	vb_out[0] = (struct vb_s *)(uintptr_t)blk;
	vb_out[1] = (struct vb_s *)(uintptr_t)vb_in; // Reuse input buffer
	base_get_frame_info(pix_format, size_out[0], &vb_out[0]->buf, vb_out[0]->phy_addr, DEFAULT_ALIGN);

	TRACE_LDC(DBG_DEBUG, "GDC usage(%d) rot(%d) src phy-addr(0x%llx) dst phy-addr(0x%llx)\n"
	, GDC_USAGE_LDC, rotation, vb_in->phy_addr, vb_out[0]->phy_addr);

	switch (rotation) {
	default:
	case ROTATION_0:
		ldc_attr = *(ldc_attr_s *)usage_param;
		if (ldc_attr.grid_info_attr.enable) {
			rotation_out[0] = ROTATION_270;
			rotation_out[1] = ROTATION_90;
		} else {
			rotation_out[0] = ROTATION_90;
			rotation_out[1] = ROTATION_270;
		}

		break;
	case ROTATION_90:
		rotation_out[0] = ROTATION_90;
		rotation_out[1] = ROTATION_0;
		break;
	case ROTATION_180:
		rotation_out[0] = ROTATION_90;
		rotation_out[1] = ROTATION_90;
		break;
	case ROTATION_270:
		rotation_out[0] = ROTATION_270;
		rotation_out[1] = ROTATION_0;
		break;
	}

	ret = init_ldc_param(vb_in, vb_out[0], ptask[0], pix_format, mesh_addr
		, false, cb_param, cb_param_size, rotation_out[0]);
	if (ret) {
		TRACE_LDC(DBG_ERR, "init ldc param 1st failed\n");
		goto FAIL_EXIT;
	}

	ret = ldc_begin_job(ctx, &data);
	if (ret) {
		TRACE_LDC(DBG_ERR, "ldc_begin_job fail, ret=%d, DWA usage(%d)\n", ret, GDC_USAGE_LDC);
		goto FAIL_EXIT;
	}

	osal_memset(ptask[0], 0, sizeof(*ptask));
	osal_memset(ptask[1], 0, sizeof(*ptask));
	ptask[0]->handle = data.handle;
	ptask[0]->rotation = rotation_out[0];

	ret = ldc_add_ldc_task(ctx, ptask[0]);
	if (ret) {
		TRACE_LDC(DBG_ERR, "ldc_add_ldc_task 1st fail, ret=%d, GDC usage(%d)\n", ret, GDC_USAGE_LDC);
		goto FAIL_EXIT;
	}

	// Reuse vb_in after 1st job assigned
	base_get_frame_info(pix_format, size_out[1], &vb_out[1]->buf, vb_out[1]->phy_addr, DEFAULT_ALIGN);

	mesh_gen_get_1st_size(size_out[0], &mesh_1st_size);
	ret = init_ldc_param(vb_out[0], vb_out[1], ptask[1], pix_format, mesh_addr + mesh_1st_size
		, true, cb_param, cb_param_size, rotation_out[1]);
	if (ret) {
		TRACE_LDC(DBG_ERR, "init ldc param 2nd failed\n");
		osal_vfree((void *)(uintptr_t)ptask[0]->private_data[2]);
		goto FAIL_EXIT;
	}

	ptask[1]->handle = data.handle;
	ptask[1]->rotation = rotation_out[1];
	ret = ldc_add_ldc_task(ctx, ptask[1]);
	if (ret) {
		TRACE_LDC(DBG_ERR, "ldc_add_ldc_task 2nd fail, ret=%d, GDC usage(%d)\n", ret, GDC_USAGE_LDC);
		goto FAIL_EXIT;
	}

	job = (struct ldc_job *)(uintptr_t)data.handle;
	job->identity.sync_io = sync_io;
	job->identity.mod_id = mod_id;
	job->identity.id = vb_in->buf.frm_num;
	p_cb_param = (struct _gdc_cb_param *)cb_param;
	snprintf(job->identity.name, sizeof(job->identity.name)
		, "dev_%d_chn_%d", p_cb_param->chn.dev_id, p_cb_param->chn.chn_id);

	ret = ldc_end_job(ctx, data.handle);

	osal_vfree(ptask[0]);
	osal_vfree(ptask[1]);
	return ret;

FAIL_EXIT:
	vb_release_block((vb_blk)(uintptr_t)blk);

FAIL_GET_VB:
FAIL_ALLOC:
	if (ptask[0])
		osal_vfree(ptask[0]);
	if (ptask[0])
		osal_vfree(ptask[1]);
	vb_release_block((vb_blk)(uintptr_t)vb_in);

	return ret;
}

int mesh_gdc_do_op(struct ldc_ctx *ctx, enum gdc_usage usage
	, const void *usage_param, struct vb_s *vb_in, pixel_format_e pix_format
	, unsigned long long mesh_addr, unsigned char sync_io, void *cb_param, unsigned int cb_param_size
	, mod_id_e mod_id, rotation_e rotation)
{
	int ret = ERR_GDC_ILLEGAL_PARAM;

	TRACE_LDC(DBG_DEBUG, "GDC usage(%d) rotation(%d), mesh-addr(0x%llx), cb_param_size(%d)\n",
			usage, rotation, (unsigned long long)mesh_addr, cb_param_size);

	if (usage != GDC_USAGE_ROTATION) {
		ret = ldc_check_null_ptr(usage_param);
		if (ret)
			return ret;
	}

	if (mesh_addr < DEFAULT_MESH_PADDR) {
		TRACE_LDC(DBG_ERR, "GDC mod(0x%x) usage(%d) mesh-addr(0x%llx) invalid\n",
			      mod_id, usage, (unsigned long long)mesh_addr);
		return -1;
	}

	switch (usage) {
	case GDC_USAGE_LDC:
		ret = mesh_gdc_do_ldc(ctx, usage_param, vb_in, pix_format, mesh_addr
			, sync_io, cb_param, cb_param_size, mod_id, rotation);
		break;
	case GDC_USAGE_ROTATION:
		ret = mesh_gdc_do_rot(ctx, vb_in, pix_format, mesh_addr
			, sync_io, cb_param, cb_param_size, mod_id, rotation);
		break;
	default:
		break;
	}

	return ret;
}
