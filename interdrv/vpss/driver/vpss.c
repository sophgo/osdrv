#include "vpss_debug.h"
#include "comm_math.h"
#include "comm_buffer.h"
#include "base_common.h"
#include "vpss_hal.h"
#include "vpss.h"
#include "vpss_uapi.h"
#include "vpss_core.h"
#include "base_cb.h"
#include "vbq.h"

static int _mesh_gdc_do_op_cb(enum gdc_usage usage, const void *usage_param,
				struct vb_s *vb_in, pixel_format_e pixel_format, unsigned long long mesh_addr,
				unsigned char sync_io, void *pcb_param, unsigned int param_size,
				mod_id_e mod_id, rotation_e rotation)
{
	struct mesh_gdc_cfg cfg;
	struct base_exe_m_cb exe_cb;

	TRACE_VPSS(DBG_DEBUG, "push jobs(%d) for gdc\n", usage);

	memset(&cfg, 0, sizeof(cfg));
	cfg.usage = usage;
	cfg.usage_param = usage_param;
	cfg.vb_in = vb_in;
	cfg.pix_format = pixel_format;
	cfg.mesh_addr = mesh_addr;
	cfg.sync_io = sync_io;
	cfg.cb_param = pcb_param;
	cfg.cb_param_size = param_size;
	cfg.rotation = rotation;

	exe_cb.callee = E_MODULE_LDC;
	exe_cb.caller = E_MODULE_VPSS;
	exe_cb.cmd_id = LDC_CB_MESH_GDC_OP;
	exe_cb.data   = &cfg;

	return base_exe_module_cb(&exe_cb);
}

/* aspect_ratio_resize: calculate the new rect to keep aspect ratio
 *   according to given in/out size.
 *
 * @param in: input video size.
 * @param out: output display size.
 *
 * @return: the rect which describe the video on output display.
 */
static rect_s aspect_ratio_resize(size_s in, size_s out)
{
	rect_s rect;
	unsigned int scale = in.height * in.width;
	unsigned int ratio_int = MIN2(out.width * in.height, out.height * in.width);
	unsigned long long height, width;

	//float ratio = MIN2((float)out.width / in.width, (float)out.height / in.height);
	//rect.height = (float)in.height * ratio + 0.5;
	//rect.width = (float)in.width * ratio + 0.5;
	//rect.x = (out.width - rect.width) >> 1;
	//rect.y = (out.height - rect.height) >> 1;

	height = (unsigned long long)in.height * ratio_int + scale/2;
	rect.height = (unsigned int)osal_div_u64(height, scale);

	width = (unsigned long long)in.width * ratio_int + scale/2;
	rect.width = (unsigned int)osal_div_u64(width, scale);

	rect.x = (out.width - rect.width) >> 1;
	rect.y = (out.height - rect.height) >> 1;

	return rect;
}

/**************************************************************************
 *   Job related APIs.
 **************************************************************************/
void vpss_gdc_callback(void *param, vb_blk blk, struct vpss_ctx *ctx)
{
	struct _vpss_gdc_cb_param *gdc_cb_param = param;
	vpss_grp grp_id;
	vpss_chn chn_id;

	if (!param)
		return;
	grp_id = gdc_cb_param->chn.dev_id;
	chn_id = gdc_cb_param->chn.chn_id;

	if (!ctx->grp_ctx[grp_id]) {
		TRACE_VPSS(DBG_NOTICE, "Grp(%d) isn't start yet.\n", grp_id);
		osal_vfree(param);
		return;
	}

	TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) usage(%d)\n", grp_id, chn_id, gdc_cb_param->usage);

	osal_atomic_set(&ctx->grp_ctx[grp_id]->chn_ctxs[chn_id].mesh.gdc_flag, 0);
	if (blk != VB_INVALID_HANDLE) {
		vb_done_handler(gdc_cb_param->chn, CHN_TYPE_OUT,
			&ctx->grp_ctx[grp_id]->vb_jobs.outs[chn_id], blk);
	}
	osal_vfree(param);
}

static unsigned char get_work_mask(struct vpss_grp_ctx *grp_ctx)
{
	unsigned char mask = 0;
	vpss_chn chn_id;

	if (!grp_ctx->is_created || !grp_ctx->is_started)
		return 0;

	for (chn_id = 0; chn_id < grp_ctx->chn_max_num; ++chn_id) {
		if (!grp_ctx->chn_ctxs[chn_id].is_enabled)
			continue;
		mask |= BIT(chn_id);
	}
	if (mask == 0)
		return 0;

	// img's mask
	mask |= BIT(7);

	return mask;
}

static void _vpss_fill_buffer(vpss_chn chn_id, struct video_buffer *grp_buf,
		uint64_t phy_addr, struct vpss_grp_ctx *grp_ctx, struct video_buffer *buf)
{
	size_s size;
	unsigned char ldc_wa = false;

	//workaround for ldc 64-align for width/height.
	if (grp_ctx->chn_ctxs[chn_id].rotation != ROTATION_0
	 || grp_ctx->chn_ctxs[chn_id].ldc_attr.enable)
		ldc_wa = true;

	if (ldc_wa) {
		size.width = ALIGN(grp_ctx->chn_ctxs[chn_id].chn_attr.width, LDC_ALIGN);
		size.height = ALIGN(grp_ctx->chn_ctxs[chn_id].chn_attr.height, LDC_ALIGN);
	} else {
		size.width = grp_ctx->chn_ctxs[chn_id].chn_attr.width;
		size.height = grp_ctx->chn_ctxs[chn_id].chn_attr.height;
	}
	base_get_frame_info(grp_ctx->chn_ctxs[chn_id].chn_attr.pixel_format
			   , size
			   , buf
			   , phy_addr
			   , grp_ctx->chn_ctxs[chn_id].align);
	buf->offset_top = 0;
	buf->offset_bottom =
		size.height - grp_ctx->chn_ctxs[chn_id].chn_attr.height;
	buf->offset_left = 0;
	buf->offset_right =
		size.width - grp_ctx->chn_ctxs[chn_id].chn_attr.width;

	if (grp_buf) {
		buf->pts = grp_buf->pts;
		buf->dev_num = grp_buf->dev_num;
		buf->frm_num = grp_buf->frm_num;
		buf->motion_lv = grp_buf->motion_lv;
		osal_memcpy(buf->motion_table, grp_buf->motion_table, MO_TBL_SIZE);
	}
}

void job_fill_buf(struct video_buffer *buf, unsigned long long *addr)
{
	unsigned char i;

	for (i = 0; i < NUM_OF_PLANES; ++i)
		addr[i] = buf->phy_addr[i];

	if (buf->pixel_format == PIXEL_FORMAT_BGR_888_PLANAR) {
		addr[0] = buf->phy_addr[2];
		addr[2] = buf->phy_addr[0];
	}
}

static int vpss_qbuf(mmf_chn_s chn, struct video_buffer *grp_buf,
	vb_blk blk, struct vpss_grp_ctx *grp_ctx, struct vpss_job *job)
{
	int ret;
	struct vb_s *vb = (struct vb_s *)(uintptr_t)blk;

	_vpss_fill_buffer(chn.chn_id, grp_buf, vb_handle2phys_addr(blk), grp_ctx, &vb->buf);

	ret = vb_qbuf(chn, CHN_TYPE_OUT, &grp_ctx->vb_jobs.outs[chn.chn_id], blk);
	if (ret != 0)
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) qbuf failed\n", chn.dev_id, chn.chn_id);
	else
		job_fill_buf(&vb->buf, job->cfg.chn_cfg[chn.chn_id].addr);

	vb_release_block(blk);

	return ret;
}

static int vpss_sb_qbuf(vpss_chn chn_id, struct vpss_chn_ctx *chn_ctx, struct vpss_job *job)
{
	u8 i = 0;

	for (i = 0; i < 3; ++i) {
		job->cfg.chn_cfg[chn_id].addr[i] = chn_ctx->sbm_ctx.phy_addr[i];
	}

	return 0;
}

static int vpss_online_qbuf(mmf_chn_s chn, void *data)
{
	int ret;
	struct vpss_job *job = (struct vpss_job *)data;
	vb_blk blk;
	struct vpss_grp_ctx *grp_ctx = (struct vpss_grp_ctx *)job->data;
	vpss_grp grp_id = chn.dev_id;
	vpss_chn chn_id = chn.chn_id;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_chn_valid(grp_id, chn_id, grp_ctx->chn_max_num);
	if (ret != 0)
		return ret;

	blk = vb_get_block_with_id(grp_ctx->chn_ctxs[chn_id].vb_pool,
		grp_ctx->chn_ctxs[chn_id].blk_size, ID_VPSS);
	if (blk == VB_INVALID_HANDLE) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) Can't acquire VB BLK for VPSS\n", grp_id, chn_id);
		return -1;
	}

	TRACE_VPSS(DBG_NOTICE, "Grp(%d) Chn(%d) acquire VB BLK\n", grp_id, chn_id);
	return vpss_qbuf(chn, NULL, blk, grp_ctx, job);
}

static int fill_buffers(struct vpss_grp_ctx *grp_ctx, struct vpss_job *job)
{
	int ret = 0;
	vb_blk blk[VPSS_MAX_CHN_NUM] = { [0 ... VPSS_MAX_CHN_NUM - 1] = VB_INVALID_HANDLE };
	vb_blk blk_grp;
	vpss_grp grp_id = grp_ctx->grp_id;
	unsigned char online_from_isp = grp_ctx->online_from_isp;
	mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = grp_id, .chn_id = 0};
	vpss_chn chn_id = 0;
	struct vpss_chn_ctx *chn_ctx;
	struct video_buffer *buf;
	struct video_buffer *buf_in = NULL;
	vb_pool pool_id = VB_INVALID_POOLID;

	if (!online_from_isp && base_mod_jobs_waitq_empty(&grp_ctx->vb_jobs.ins))
		return ERR_VPSS_BUF_EMPTY;

	// get buffers.
	for (chn_id = 0; chn_id < VPSS_MAX_CHN_NUM; ++chn_id) {
		chn_ctx = &grp_ctx->chn_ctxs[chn_id];
		if (!chn_ctx->is_enabled)
			continue;
		if (chn_ctx->is_drop)
			continue;
		if (chn_ctx->buf_wrap.enable)
			continue;

		chn.chn_id = chn_id;
		job->cfg.chn_cfg[chn_id].addr[0] = 0;

		// chn buffer from user
		if (!base_mod_jobs_waitq_empty(&grp_ctx->vb_jobs.outs[chn_id])) {
			TRACE_VPSS(DBG_DEBUG, "Grp(%d) Chn(%d) chn buffer from user.\n", grp_id, chn_id);

			buf = base_mod_jobs_enque_work(&grp_ctx->vb_jobs.outs[chn_id]);
			if (!buf) {
				TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) qbuf failed.\n", grp_id, chn_id);
				ret = ERR_VPSS_NOTREADY;
				break;
			}

			// Implement qbuf in user space
			job_fill_buf(buf, job->cfg.chn_cfg[chn_id].addr);
			job->cfg.chn_cfg[chn_id].bytesperline[0] = buf->stride[0];
			job->cfg.chn_cfg[chn_id].bytesperline[1] = buf->stride[1];
			continue;
		}

		// chn buffer from pool
		blk[chn_id] = vb_get_block_with_id(chn_ctx->vb_pool, chn_ctx->blk_size, ID_VPSS);
		if (blk[chn_id] == VB_INVALID_HANDLE) {
			if (online_from_isp) {
				if (chn_ctx->vb_pool == VB_INVALID_POOLID)
					pool_id = find_vb_pool(chn_ctx->blk_size);
				else
					pool_id = chn_ctx->vb_pool;
				vb_acquire_block(vpss_online_qbuf, chn, pool_id, job);
				TRACE_VPSS(DBG_INFO, "Grp(%d) Chn(%d) acquire VB BLK later\n"
					, grp_id, chn_id);
			} else {
				TRACE_VPSS(DBG_ERR, "Grp(%d) Chn(%d) Can't acquire VB BLK for VPSS\n"
					, grp_id, chn_id);
				ret = ERR_VPSS_NOBUF;
				break;
			}
		}
	}
	if (ret != 0)
		goto ERR_FILL_BUF;

	if (!online_from_isp) {
		buf_in = base_mod_jobs_enque_work(&grp_ctx->vb_jobs.ins);
		if (buf_in == NULL) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) qbuf failed.\n", grp_id);
			ret = ERR_VPSS_NOTREADY;
			goto ERR_FILL_BUF;
		}

		TRACE_VPSS(DBG_DEBUG, "grp(%d) buf: 0x%lx-0x%lx-0x%lx\n", grp_id,
			(unsigned long)buf_in->phy_addr[0], (unsigned long)buf_in->phy_addr[1],
			(unsigned long)buf_in->phy_addr[2]);

		job_fill_buf(buf_in, job->cfg.grp_cfg.addr);
	}

	for (chn_id = 0; chn_id < VPSS_MAX_CHN_NUM; ++chn_id) {
		chn.chn_id = chn_id;

		if (grp_ctx->chn_ctxs[chn_id].buf_wrap.enable) {
			vpss_sb_qbuf(chn_id, &grp_ctx->chn_ctxs[chn_id], job);
			continue;
		}

		if (blk[chn_id] == VB_INVALID_HANDLE)
			continue;

		vpss_qbuf(chn, buf_in, blk[chn_id], grp_ctx, job);
	}

	return ret;
ERR_FILL_BUF:
	while ((chn_id > 0) && (--chn_id < VPSS_MAX_CHN_NUM)) {
		if (blk[chn_id] != VB_INVALID_HANDLE)
			vb_release_block(blk[chn_id]);
	}
	blk_grp = base_mod_jobs_waitq_pop(&grp_ctx->vb_jobs.ins);
	if (blk_grp != VB_INVALID_HANDLE)
		vb_release_block(blk_grp);

	return ret;
}

void vpss_chn_cancel_block(struct vpss_grp_ctx *grp_ctx, vpss_chn chn_id)
{
	vpss_grp grp_id = grp_ctx->grp_id;
	mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = grp_id, .chn_id = chn_id};
	vb_pool pool_id = VB_INVALID_POOLID;

	if (grp_ctx->chn_ctxs[chn_id].vb_pool == VB_INVALID_POOLID)
		pool_id = find_vb_pool(grp_ctx->chn_ctxs[chn_id].blk_size);
	else
		pool_id = grp_ctx->chn_ctxs[chn_id].vb_pool;
	vb_cancel_block(chn, pool_id);
}

void release_buffers(struct vpss_grp_ctx *grp_ctx)
{
	vpss_grp grp_id = grp_ctx->grp_id;
	mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = grp_id, .chn_id = 0};
	vb_blk blk;
	vpss_chn chn_id;

	if (!grp_ctx->online_from_isp) {
		chn.chn_id = 0;
		if (!base_mod_jobs_workq_empty(&grp_ctx->vb_jobs.ins)) {
			vb_dqbuf(chn, &grp_ctx->vb_jobs.ins, &blk);
			if (blk != VB_INVALID_HANDLE)
				vb_release_block(blk);
		}
	}

	for (chn_id = 0; chn_id < VPSS_MAX_CHN_NUM; ++chn_id) {
		struct vpss_chn_ctx *chn_ctx = &grp_ctx->chn_ctxs[chn_id];

		if (!chn_ctx->is_enabled)
			continue;
		if (chn_ctx->buf_wrap.enable)
			continue;

		vpss_chn_cancel_block(grp_ctx, chn_id);

		chn.chn_id = chn_id;
		while (!base_mod_jobs_workq_empty(&grp_ctx->vb_jobs.outs[chn_id])) {
			vb_dqbuf(chn, &grp_ctx->vb_jobs.outs[chn_id], &blk);
			if (blk != VB_INVALID_HANDLE)
				vb_release_block(blk);
		}
	}
}

static void _release_vpss_waitq(mmf_chn_s chn, enum chn_type_e chn_type, struct vpss_grp_ctx *grp_ctx)
{
	vb_blk blk_grp;

	if (chn_type == CHN_TYPE_OUT)
		blk_grp = base_mod_jobs_waitq_pop(&grp_ctx->vb_jobs.outs[chn.chn_id]);
	else
		blk_grp = base_mod_jobs_waitq_pop(&grp_ctx->vb_jobs.ins);

	if (blk_grp != VB_INVALID_HANDLE)
		vb_release_block(blk_grp);
}

static int _vpss_online_get_dpcm_wr_crop(unsigned char snr_num,
	rect_s *dpcm_wr_crop, size_s src_size)
{
#if 0
	struct crop_size crop = g_dpcm_wr_i.dpcm_wr_i_crop[snr_num];
	int ret = 0;

	if (g_dpcm_wr_i.dpcm_wr_i_dpcmon) {
		// check if dpcm_wr crop valid
		if (crop.end_x <= crop.start_x ||
			crop.end_y <= crop.start_y ||
			crop.end_x > src_size.width ||
			crop.end_y > src_size.height ||
			((unsigned int)(crop.end_x - crop.start_x) == src_size.width &&
			(unsigned int)(crop.end_y - crop.start_y) == src_size.height))
			ret = ERR_VPSS_ILLEGAL_PARAM;
		else {
			dpcm_wr_crop->x = (int)crop.start_x;
			dpcm_wr_crop->y = (int)crop.start_y;
			dpcm_wr_crop->width = (unsigned int)(crop.end_x - crop.start_x);
			dpcm_wr_crop->height = (unsigned int)(crop.end_y - crop.start_y);
		}
	} else
		ret = ERR_VPSS_NOT_PERM;

	return ret;
#else
	if (snr_num >= VPSS_MAX_GRP_NUM || !dpcm_wr_crop || !src_size.width)
		return -1;

	return ERR_VPSS_NOT_PERM;
#endif
}

/*
 * _vpss_get_union_crop() - get union crop area of crop_a & crop_b.
 * If two crop area has no union, return crop_a
 */
static rect_s _vpss_get_union_crop(rect_s crop_a, rect_s crop_b)
{
	rect_s union_crop;

	// check if no union
	if ((crop_a.x >= crop_b.x + (int)crop_b.width) ||
		(crop_a.y >= crop_b.y + (int)crop_b.height) ||
		(crop_b.x >= crop_a.x + (int)crop_a.width) ||
		(crop_b.y >= crop_a.y + (int)crop_a.height))
		return crop_a;

	union_crop.x = (crop_a.x > crop_b.x) ? crop_a.x : crop_b.x;
	union_crop.y = (crop_a.y > crop_b.y) ? crop_a.y : crop_b.y;
	union_crop.width =
		((crop_a.x + (int)crop_a.width) < (crop_b.x + (int)crop_b.width)) ?
		(unsigned int)(crop_a.x + (int)crop_a.width - union_crop.x) :
		(unsigned int)(crop_b.x + (int)crop_b.width - union_crop.x);
	union_crop.height =
		((crop_a.y + (int)crop_a.height) < (crop_b.y + (int)crop_b.height)) ?
		(unsigned int)(crop_a.y + (int)crop_a.height - union_crop.y) :
		(unsigned int)(crop_b.y + (int)crop_b.height - union_crop.y);

	return union_crop;
}

/* _is_frame_crop_changed() - to see if frame's crop info changed
 */
static unsigned char _is_frame_crop_changed(vpss_grp grp_id, struct vpss_grp_ctx *grp_ctx)
{
	struct vb_s *vb_in = NULL;
	unsigned char ret = false;
	struct vb_jobs_t *jobs;

	if (base_mod_jobs_waitq_empty(&grp_ctx->vb_jobs.ins))
		return false;

	jobs = &grp_ctx->vb_jobs.ins;
	FIFO_GET_FRONT(&jobs->waitq, &vb_in);

	if (vb_in == NULL) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) unexpected empty waitq\n", grp_id);
		return false;
	}

	//GDC 64 align case
	if ((grp_ctx->offset_left != vb_in->buf.offset_left) ||
		(grp_ctx->offset_top != vb_in->buf.offset_top) ||
		(grp_ctx->offset_right != vb_in->buf.offset_right) ||
		(grp_ctx->offset_bottom != vb_in->buf.offset_bottom)) {
		grp_ctx->offset_left = vb_in->buf.offset_left;
		grp_ctx->offset_top = vb_in->buf.offset_top;
		grp_ctx->offset_right = vb_in->buf.offset_right;
		grp_ctx->offset_bottom = vb_in->buf.offset_bottom;
		ret = true;
	}

	//dis case
	if (memcmp(&vb_in->buf.frame_crop, &grp_ctx->frame_crop, sizeof(vb_in->buf.frame_crop))) {
		unsigned char chk_width_even = IS_FMT_YUV420(grp_ctx->grp_attr.pixel_format) ||
				      IS_FMT_YUV422(grp_ctx->grp_attr.pixel_format);
		unsigned char chk_height_even = IS_FMT_YUV420(grp_ctx->grp_attr.pixel_format);

		if (chk_width_even && ((vb_in->buf.frame_crop.end_x - vb_in->buf.frame_crop.start_x) & 0x01)) {
			TRACE_VPSS(DBG_WARN, "grp_id(%d) frame-crop invalid - start_x(%d) end_x(%d)\n",
				       grp_id, vb_in->buf.frame_crop.start_x, vb_in->buf.frame_crop.end_x);
			TRACE_VPSS(DBG_WARN, "frame-crop's width should be even for yuv format\n");
			return ret;
		}
		if (chk_height_even && ((vb_in->buf.frame_crop.end_y - vb_in->buf.frame_crop.start_y) & 0x01)) {
			TRACE_VPSS(DBG_WARN, "grp_id(%d) frame-crop invalid - start_y(%d) end_y(%d)\n",
				       grp_id, vb_in->buf.frame_crop.start_y, vb_in->buf.frame_crop.end_y);
			TRACE_VPSS(DBG_WARN, "frame-crop's height should be even for yuv format\n");
			return ret;
		}

		grp_ctx->frame_crop = vb_in->buf.frame_crop;
		ret = true;
	}
	return ret;
}

/* _is_frame_crop_valid() - to see if frame's crop info valid/enabled
 */
static bool _is_frame_crop_valid(struct vpss_grp_ctx *grp_ctx)
{
	return (grp_ctx->frame_crop.end_x > grp_ctx->frame_crop.start_x &&
		grp_ctx->frame_crop.end_y > grp_ctx->frame_crop.start_y &&
		grp_ctx->frame_crop.end_x <= grp_ctx->grp_attr.w &&
		grp_ctx->frame_crop.end_y <= grp_ctx->grp_attr.h &&
		!((unsigned int)(grp_ctx->frame_crop.end_x - grp_ctx->frame_crop.start_x)
			== grp_ctx->grp_attr.w &&
		  (unsigned int)(grp_ctx->frame_crop.end_y - grp_ctx->frame_crop.start_y)
			== grp_ctx->grp_attr.h));
}

static void _vpss_over_crop_resize
	(struct vpss_img_in_cfg *grp_hw_cfg, rect_s crop_rect, rect_s *resize_rect)
{
	unsigned int scale = crop_rect.width * crop_rect.height;
	unsigned int ratio;

	TRACE_VPSS(DBG_DEBUG, "rect_crop (l=%d, t=%d, w=%d, h=%d)\n",
			grp_hw_cfg->crop.left,
			grp_hw_cfg->crop.top,
			grp_hw_cfg->crop.width,
			grp_hw_cfg->crop.height);
	TRACE_VPSS(DBG_DEBUG, "rect before resize(%d, %d, %d, %d)\n"
				, resize_rect->x, resize_rect->y
				, resize_rect->width, resize_rect->height);

	if (crop_rect.x < 0) {
		//ratio = (float)ABS(crop_rect.x) / crop_rect.width;
		//resize_rect->x += (int)(resize_rect->width * ratio + 0.5);
		//resize_rect->width -= (unsigned int)(resize_rect->width * ratio + 0.5);
		ratio = ABS(crop_rect.x) * crop_rect.height;
		resize_rect->x += (int)(resize_rect->width * ratio + scale / 2) / scale;
		resize_rect->width -= (unsigned int)(resize_rect->width * ratio + scale / 2) / scale;
	}

	if (crop_rect.x + crop_rect.width > grp_hw_cfg->crop.width) {
		//ratio = (float)(crop_rect.x + crop_rect.width - grp_hw_cfg->rect_crop.width)
		//	/ (crop_rect.width);
		//resize_rect->width -= (unsigned int)(resize_rect->width * ratio + 0.5);
		ratio = (crop_rect.x + crop_rect.width - grp_hw_cfg->crop.width)
			* (crop_rect.height);
		resize_rect->width -= (unsigned int)(resize_rect->width * ratio + scale / 2) / scale;
	}

	if (crop_rect.y < 0) {
		//ratio = (float)ABS(crop_rect.y) / crop_rect.height;
		//resize_rect->y += (int)(resize_rect->height * ratio + 0.5);
		//resize_rect->height -= (unsigned int)(resize_rect->height * ratio + 0.5);
		ratio = ABS(crop_rect.y) * crop_rect.width;
		resize_rect->y += (int)(resize_rect->height * ratio + scale / 2) / scale;
		resize_rect->height -= (unsigned int)(resize_rect->height * ratio + scale / 2) / scale;
	}

	if (crop_rect.y + crop_rect.height > grp_hw_cfg->crop.height) {
		//ratio = (float)(crop_rect.y + crop_rect.height - grp_hw_cfg->rect_crop.height)
		//	/ (crop_rect.height);
		//resize_rect->height -= (unsigned int)(resize_rect->height * ratio + 0.5);
		ratio = (crop_rect.y + crop_rect.height - grp_hw_cfg->crop.height)
			* (crop_rect.width);
		resize_rect->height -= (unsigned int)(resize_rect->height * ratio + scale / 2) / scale;
	}

	TRACE_VPSS(DBG_INFO, "rect after resize(%d, %d, %d, %d)\n"
			, resize_rect->x, resize_rect->y
			, resize_rect->width, resize_rect->height);
}

/*
 * @param chn_id: VPSS Chn to update cfg
 * @param ctx: VPSS ctx which records settings of this grp
 */
void _vpss_chn_hw_cfg_update(vpss_chn chn_id, struct vpss_grp_ctx *grp_ctx)
{
	unsigned char i;
	vpss_grp grp_id = grp_ctx->grp_id;
	unsigned char online_from_isp = grp_ctx->online_from_isp;
	struct vpss_chn_ctx *chn_ctx = &grp_ctx->chn_ctxs[chn_id];
	struct vpss_img_in_cfg *grp_hw_cfg = &grp_ctx->hw_cfg.grp_cfg;
	struct vpss_sc_cfg *hw_chn_cfg = &grp_ctx->hw_cfg.chn_cfg[chn_id];
	vb_cal_config_s vb_cal_config;
	rect_s chn_crop = chn_ctx->crop_info.crop_rect;
	unsigned char crop_over_src_range = false;

	common_getpicbufferconfig(chn_ctx->chn_attr.width, chn_ctx->chn_attr.height,
		chn_ctx->chn_attr.pixel_format, DATA_BITWIDTH_8
		, COMPRESS_MODE_NONE, chn_ctx->align, &vb_cal_config);
	hw_chn_cfg->bytesperline[0] = vb_cal_config.main_stride;
	hw_chn_cfg->bytesperline[1] = vb_cal_config.c_stride;

	if (chn_ctx->crop_info.enable) {
		//FLOAT h_ratio = 1.0f, v_ratio = 1.0f;
		unsigned int scale = grp_ctx->grp_attr.w * grp_ctx->grp_attr.h;
		unsigned int h_ratio = scale, v_ratio = scale;
		unsigned long long left, right, height, width;

		if (!online_from_isp) {
			// use ratio-coordinate if dis enabled.
			if (_is_frame_crop_valid(grp_ctx)) {
				//h_ratio = (FLOAT)grp_hw_cfg->rect_crop.width / ctx->grp_attr.w;
				//v_ratio = (FLOAT)grp_hw_cfg->rect_crop.height / ctx->grp_attr.h;
				h_ratio = grp_hw_cfg->crop.width * grp_ctx->grp_attr.h;
				v_ratio = grp_hw_cfg->crop.height * grp_ctx->grp_attr.w;
			}
		} else {
			rect_s dpcm_wr_crop;
			size_s chn_src_size;
			int ret;

			chn_src_size.width = grp_hw_cfg->crop.width;
			chn_src_size.height = grp_hw_cfg->crop.height;
			ret = _vpss_online_get_dpcm_wr_crop(grp_id, &dpcm_wr_crop, chn_src_size);
			if (ret == 0)
				chn_crop = _vpss_get_union_crop(dpcm_wr_crop, chn_crop);
		}
		//hw_chn_cfg->rect_crop.left = chn_crop.x = chn_crop.x * h_ratio;
		//hw_chn_cfg->rect_crop.top = chn_crop.y = chn_crop.y * v_ratio;
		//hw_chn_cfg->rect_crop.width = chn_crop.width = chn_crop.width * h_ratio;
		//hw_chn_cfg->rect_crop.height = chn_crop.height = chn_crop.height * v_ratio;
		left = chn_crop.x * (s64)h_ratio;
		left = osal_div_u64(left, scale);
		hw_chn_cfg->crop.left = chn_crop.x = left;

		right = chn_crop.y * (s64)v_ratio;
		right = osal_div_u64(right, scale);
		hw_chn_cfg->crop.top = chn_crop.y = right;

		width = chn_crop.width * (s64)h_ratio;
		width = osal_div_u64(width, scale);
		hw_chn_cfg->crop.width = chn_crop.width = (unsigned int)width;

		height = chn_crop.height * (s64)v_ratio;
		height = osal_div_u64(height, scale);
		hw_chn_cfg->crop.height = chn_crop.height = (unsigned int)height;

		// check if crop rect contains the region outside input src
		if (chn_crop.x < 0) {
			hw_chn_cfg->crop.left = 0;
			hw_chn_cfg->crop.width = (chn_crop.width - ABS(chn_crop.x));
			crop_over_src_range = true;
		}
		if (chn_crop.x + chn_crop.width > grp_hw_cfg->crop.width) {
			hw_chn_cfg->crop.width = grp_hw_cfg->crop.width - hw_chn_cfg->crop.left;
			crop_over_src_range = true;
		}

		if (chn_crop.y < 0) {
			hw_chn_cfg->crop.top = 0;
			hw_chn_cfg->crop.height = (chn_crop.height - ABS(chn_crop.y));
			crop_over_src_range = true;
		}
		if (chn_crop.y + chn_crop.height > grp_hw_cfg->crop.height) {
			hw_chn_cfg->crop.height = grp_hw_cfg->crop.height - hw_chn_cfg->crop.top;
			crop_over_src_range = true;
		}
	} else {
		hw_chn_cfg->crop.left = hw_chn_cfg->crop.top
			= chn_crop.x = chn_crop.y = 0;
		hw_chn_cfg->crop.width = chn_crop.width = grp_hw_cfg->crop.width;
		hw_chn_cfg->crop.height = chn_crop.height = grp_hw_cfg->crop.height;
		if (online_from_isp) {
			rect_s dpcm_wr_crop;
			size_s chn_src_size;
			int ret;

			chn_src_size.width = grp_hw_cfg->crop.width;
			chn_src_size.height = grp_hw_cfg->crop.height;
			ret = _vpss_online_get_dpcm_wr_crop(grp_id, &dpcm_wr_crop, chn_src_size);
			if (ret == 0) {
				hw_chn_cfg->crop.left = chn_crop.x = dpcm_wr_crop.x;
				hw_chn_cfg->crop.top = chn_crop.y = dpcm_wr_crop.y;
				hw_chn_cfg->crop.width = chn_crop.width = dpcm_wr_crop.width;
				hw_chn_cfg->crop.height = chn_crop.height = dpcm_wr_crop.height;
			}
		}
	}
	TRACE_VPSS(DBG_DEBUG, "grp(%d) chn(%d) rect(%d %d %d %d)\n", grp_id, chn_id
			, hw_chn_cfg->crop.left, hw_chn_cfg->crop.top
			, hw_chn_cfg->crop.width, hw_chn_cfg->crop.height);

	if (chn_ctx->chn_attr.aspect_ratio.mode == ASPECT_RATIO_AUTO) {
		size_s in, out;
		rect_s rect;
		unsigned char is_border_enabled = false;

		in.width = chn_crop.width;
		in.height = chn_crop.height;
		out.width = chn_ctx->chn_attr.width;
		out.height = chn_ctx->chn_attr.height;
		rect = aspect_ratio_resize(in, out);

		if (crop_over_src_range)
			_vpss_over_crop_resize(grp_hw_cfg, chn_crop, &rect);

		is_border_enabled = chn_ctx->chn_attr.aspect_ratio.enable_bgcolor
			&& ((rect.width != chn_ctx->chn_attr.width)
			 || (rect.height != chn_ctx->chn_attr.height));

		TRACE_VPSS(DBG_INFO, "input(%d %d) output(%d %d)\n"
				, in.width, in.height, out.width, out.height);
		TRACE_VPSS(DBG_INFO, "ratio (%d %d %d %d) border_enabled(%d)\n"
				, rect.x, rect.y, rect.width, rect.height, is_border_enabled);

		hw_chn_cfg->border_cfg.enable = is_border_enabled;
		hw_chn_cfg->border_cfg.offset_x = rect.x;
		hw_chn_cfg->border_cfg.offset_y = rect.y;
		hw_chn_cfg->border_cfg.bg_color[2] = chn_ctx->chn_attr.aspect_ratio.bgcolor & 0xff;
		hw_chn_cfg->border_cfg.bg_color[1] = (chn_ctx->chn_attr.aspect_ratio.bgcolor >> 8) & 0xff;
		hw_chn_cfg->border_cfg.bg_color[0] = (chn_ctx->chn_attr.aspect_ratio.bgcolor >> 16) & 0xff;

		if (is_border_enabled) {
			hw_chn_cfg->dst_rect.left = hw_chn_cfg->dst_rect.top = 0;
		} else {
			hw_chn_cfg->dst_rect.left = rect.x;
			hw_chn_cfg->dst_rect.top = rect.y;
		}


		if (IS_FMT_YUV420(chn_ctx->chn_attr.pixel_format)
			|| IS_FMT_YUV422(chn_ctx->chn_attr.pixel_format)) {
			hw_chn_cfg->dst_rect.width = rect.width & ~0x01;
			hw_chn_cfg->dst_rect.left &= ~0x01;
		} else
			hw_chn_cfg->dst_rect.width = rect.width;

		if (IS_FMT_YUV420(chn_ctx->chn_attr.pixel_format))
			hw_chn_cfg->dst_rect.height = rect.height & ~0x01;
		else
			hw_chn_cfg->dst_rect.height = rect.height;
	} else if (chn_ctx->chn_attr.aspect_ratio.mode == ASPECT_RATIO_MANUAL) {
		rect_s rect = chn_ctx->chn_attr.aspect_ratio.video_rect;
		unsigned char is_border_enabled = false;

		if (crop_over_src_range)
			_vpss_over_crop_resize(grp_hw_cfg, chn_crop, &rect);

		is_border_enabled = chn_ctx->chn_attr.aspect_ratio.enable_bgcolor
			&& ((rect.width != chn_ctx->chn_attr.width)
			 || (rect.height != chn_ctx->chn_attr.height));

		TRACE_VPSS(DBG_INFO, "rect(%d %d %d %d) border_enabled(%d)\n"
				, rect.x, rect.y, rect.width, rect.height, is_border_enabled);

		if (is_border_enabled) {
			hw_chn_cfg->dst_rect.left = hw_chn_cfg->dst_rect.top = 0;
		} else {
			hw_chn_cfg->dst_rect.left = rect.x;
			hw_chn_cfg->dst_rect.top = rect.y;
		}
		hw_chn_cfg->dst_rect.width = rect.width;
		hw_chn_cfg->dst_rect.height = rect.height;

		hw_chn_cfg->border_cfg.enable = is_border_enabled;
		hw_chn_cfg->border_cfg.offset_x = rect.x;
		hw_chn_cfg->border_cfg.offset_y = rect.y;
		hw_chn_cfg->border_cfg.bg_color[2] = chn_ctx->chn_attr.aspect_ratio.bgcolor & 0xff;
		hw_chn_cfg->border_cfg.bg_color[1] = (chn_ctx->chn_attr.aspect_ratio.bgcolor >> 8) & 0xff;
		hw_chn_cfg->border_cfg.bg_color[0] = (chn_ctx->chn_attr.aspect_ratio.bgcolor >> 16) & 0xff;
	} else {
		rect_s rect;

		rect.x = rect.y = 0;
		rect.width = chn_ctx->chn_attr.width;
		rect.height = chn_ctx->chn_attr.height;
		if (crop_over_src_range)
			_vpss_over_crop_resize(grp_hw_cfg, chn_crop, &rect);

		hw_chn_cfg->dst_rect.left = hw_chn_cfg->dst_rect.top = 0;
		hw_chn_cfg->dst_rect.width = rect.width;
		hw_chn_cfg->dst_rect.height = rect.height;
		if (crop_over_src_range) {
			hw_chn_cfg->border_cfg.enable = true;
			hw_chn_cfg->border_cfg.offset_x = rect.x;
			hw_chn_cfg->border_cfg.offset_y = rect.y;
			hw_chn_cfg->border_cfg.bg_color[2] = 0;
			hw_chn_cfg->border_cfg.bg_color[1] = 0;
			hw_chn_cfg->border_cfg.bg_color[0] = 0;
		} else {
			hw_chn_cfg->border_cfg.enable = false;
		}
	}

	if (hw_chn_cfg->dst_rect.width * VPSS_MAX_ZOOMOUT  < hw_chn_cfg->crop.width
		|| hw_chn_cfg->dst_rect.height * VPSS_MAX_ZOOMOUT  < hw_chn_cfg->crop.height) {
		TRACE_VPSS(DBG_ERR, "zoom out over %d times, sc in(w:%d, h:%d), sc out(w:%d, h:%d)\n"
			, VPSS_MAX_ZOOMOUT, hw_chn_cfg->crop.width, hw_chn_cfg->crop.height
			, hw_chn_cfg->dst_rect.width, hw_chn_cfg->dst_rect.height);

		hw_chn_cfg->crop.width = hw_chn_cfg->dst_rect.width * VPSS_MAX_ZOOMOUT;
		hw_chn_cfg->crop.height = hw_chn_cfg->dst_rect.height * VPSS_MAX_ZOOMOUT;
		TRACE_VPSS(DBG_ERR, "Modify to sc in(w:%d, h:%d), sc out(w:%d, h:%d)\n"
			, hw_chn_cfg->crop.width, hw_chn_cfg->crop.height
			, hw_chn_cfg->dst_rect.width, hw_chn_cfg->dst_rect.height);
	}

	hw_chn_cfg->quant_cfg.enable = chn_ctx->chn_attr.normalize.enable;
	if (chn_ctx->chn_attr.normalize.enable) {
		struct vpss_int_normalize *int_norm =
			(struct vpss_int_normalize *)&grp_ctx->chn_ctxs[chn_id].chn_attr.normalize;

		for (i = 0; i < 3; i++) {
			hw_chn_cfg->quant_cfg.sc_frac[i] = int_norm->sc_frac[i];
			hw_chn_cfg->quant_cfg.sub[i] = int_norm->sub[i];
			hw_chn_cfg->quant_cfg.sub_frac[i] = int_norm->sub_frac[i];
		}

		TRACE_VPSS(DBG_DEBUG, "sc_frac(0x%x, 0x%x, 0x%x)\n",
			hw_chn_cfg->quant_cfg.sc_frac[0],
			hw_chn_cfg->quant_cfg.sc_frac[1],
			hw_chn_cfg->quant_cfg.sc_frac[2]);

		TRACE_VPSS(DBG_DEBUG, "sub(0x%x, 0x%x, 0x%x), sub_frac(0x%x, 0x%x, 0x%x)\n",
			hw_chn_cfg->quant_cfg.sub[0],
			hw_chn_cfg->quant_cfg.sub[1],
			hw_chn_cfg->quant_cfg.sub[2],
			hw_chn_cfg->quant_cfg.sub_frac[0],
			hw_chn_cfg->quant_cfg.sub_frac[1],
			hw_chn_cfg->quant_cfg.sub_frac[2]);

		hw_chn_cfg->quant_cfg.rounding = (enum sc_quant_rounding)int_norm->rounding;
	} else {
		hw_chn_cfg->y_ratio = chn_ctx->y_ratio;
	}

	switch (chn_ctx->coef) {
	default:
	case VPSS_SCALE_COEF_BICUBIC:
		hw_chn_cfg->sc_coef = SC_SCALING_COEF_BICUBIC;
		break;
	case VPSS_SCALE_COEF_BILINEAR:
		hw_chn_cfg->sc_coef = SC_SCALING_COEF_BILINEAR;
		break;
	case VPSS_SCALE_COEF_NEAREST:
		hw_chn_cfg->sc_coef = SC_SCALING_COEF_NEAREST;
		break;
	case VPSS_SCALE_COEF_BICUBIC_OPENCV:
		hw_chn_cfg->sc_coef = SC_SCALING_COEF_BICUBIC_OPENCV;
		break;
	}

	for (i = 0; i < VPSS_RECT_NUM; i++) {
		rect_s rect;
		unsigned short thick;

		hw_chn_cfg->border_vpp_cfg[i].enable = chn_ctx->draw_rect.rects[i].enable;
		if (hw_chn_cfg->border_vpp_cfg[i].enable) {
			hw_chn_cfg->border_vpp_cfg[i].bg_color[0] = (chn_ctx->draw_rect.rects[i].bg_color >> 16) & 0xff;
			hw_chn_cfg->border_vpp_cfg[i].bg_color[1] = (chn_ctx->draw_rect.rects[i].bg_color >> 8) & 0xff;
			hw_chn_cfg->border_vpp_cfg[i].bg_color[2] = chn_ctx->draw_rect.rects[i].bg_color & 0xff;

			rect = chn_ctx->draw_rect.rects[i].rect;
			thick = chn_ctx->draw_rect.rects[i].thick;
			if ((rect.x + rect.width) > chn_ctx->chn_attr.width)
				rect.width = chn_ctx->chn_attr.width - rect.x;
			if ((rect.y + rect.height) > chn_ctx->chn_attr.height)
				rect.height = chn_ctx->chn_attr.height - rect.y;

			hw_chn_cfg->border_vpp_cfg[i].outside.start_x = rect.x;
			hw_chn_cfg->border_vpp_cfg[i].outside.start_y = rect.y;
			hw_chn_cfg->border_vpp_cfg[i].outside.end_x = rect.x + rect.width;
			hw_chn_cfg->border_vpp_cfg[i].outside.end_y = rect.y + rect.height;
			hw_chn_cfg->border_vpp_cfg[i].inside.start_x = rect.x + thick;
			hw_chn_cfg->border_vpp_cfg[i].inside.start_y = rect.y + thick;
			hw_chn_cfg->border_vpp_cfg[i].inside.end_x =
				hw_chn_cfg->border_vpp_cfg[i].outside.end_x - thick;
			hw_chn_cfg->border_vpp_cfg[i].inside.end_y =
				hw_chn_cfg->border_vpp_cfg[i].outside.end_y - thick;
		}
	}

	hw_chn_cfg->convert_to_cfg.enable = chn_ctx->convert.enable;
	if (hw_chn_cfg->convert_to_cfg.enable) {
		hw_chn_cfg->convert_to_cfg.a_frac[0] = chn_ctx->convert.a_factor[0];
		hw_chn_cfg->convert_to_cfg.a_frac[1] = chn_ctx->convert.a_factor[1];
		hw_chn_cfg->convert_to_cfg.a_frac[2] = chn_ctx->convert.a_factor[2];
		hw_chn_cfg->convert_to_cfg.b_frac[0] = chn_ctx->convert.b_factor[0];
		hw_chn_cfg->convert_to_cfg.b_frac[1] = chn_ctx->convert.b_factor[1];
		hw_chn_cfg->convert_to_cfg.b_frac[2] = chn_ctx->convert.b_factor[2];
	}

	osal_memcpy(&hw_chn_cfg->csc_cfg, &chn_ctx->csc, sizeof(hw_chn_cfg->csc_cfg));

	TRACE_VPSS(DBG_DEBUG, "hw_chn_cfg coef[0][0]: %#4x coef[0][1]: %#4x coef[0][2]: %#4x\n"
		, hw_chn_cfg->csc_cfg.coef[0][0]
		, hw_chn_cfg->csc_cfg.coef[0][1]
		, hw_chn_cfg->csc_cfg.coef[0][2]);
	TRACE_VPSS(DBG_DEBUG, "coef[1][0]: %#4x coef[1][1]: %#4x coef[1][2]: %#4x\n"
		, hw_chn_cfg->csc_cfg.coef[1][0]
		, hw_chn_cfg->csc_cfg.coef[1][1]
		, hw_chn_cfg->csc_cfg.coef[1][2]);
	TRACE_VPSS(DBG_DEBUG, "coef[2][0]: %#4x coef[2][1]: %#4x coef[2][2]: %#4x\n"
		, hw_chn_cfg->csc_cfg.coef[2][0]
		, hw_chn_cfg->csc_cfg.coef[2][1]
		, hw_chn_cfg->csc_cfg.coef[2][2]);
	TRACE_VPSS(DBG_DEBUG, "sub[0]: %3d sub[1]: %3d sub[2]: %3d\n"
		, hw_chn_cfg->csc_cfg.sub[0]
		, hw_chn_cfg->csc_cfg.sub[1]
		, hw_chn_cfg->csc_cfg.sub[2]);
	TRACE_VPSS(DBG_DEBUG, "add[0]: %3d add[1]: %3d add[2]: %3d\n"
		, hw_chn_cfg->csc_cfg.add[0]
		, hw_chn_cfg->csc_cfg.add[1]
		, hw_chn_cfg->csc_cfg.add[2]);
}

/*
 * @param grp_ctx: VPSS ctx which records settings of this grp
 * @param grp_hw_cfg: cfg to be updated
 */
void _vpss_grp_hw_cfg_update(struct vpss_grp_ctx *grp_ctx)
{
	vpss_grp grp_id = grp_ctx->grp_id;
	vb_cal_config_s vb_cal_config;
	struct vpss_img_in_cfg *grp_hw_cfg = &grp_ctx->hw_cfg.grp_cfg;

	common_getpicbufferconfig(grp_ctx->grp_attr.w, grp_ctx->grp_attr.h,
		grp_ctx->grp_attr.pixel_format, DATA_BITWIDTH_8
		, COMPRESS_MODE_NONE, DEFAULT_ALIGN, &vb_cal_config);
	grp_hw_cfg->bytesperline[0] = vb_cal_config.main_stride;
	grp_hw_cfg->bytesperline[1] = vb_cal_config.c_stride;

	// frame_crop applied if valid
	if (_is_frame_crop_valid(grp_ctx)) {
		// for frame crop.
		rect_s grp_crop;

		grp_crop.x = grp_ctx->frame_crop.start_x;
		grp_crop.y = grp_ctx->frame_crop.start_y;
		grp_crop.width = grp_ctx->frame_crop.end_x - grp_ctx->frame_crop.start_x;
		grp_crop.height = grp_ctx->frame_crop.end_y - grp_ctx->frame_crop.start_y;
		if (grp_ctx->grp_crop_info.enable)
			grp_crop = _vpss_get_union_crop(grp_crop, grp_ctx->grp_crop_info.crop_rect);

		grp_hw_cfg->crop.left = grp_crop.x + grp_ctx->offset_left;
		grp_hw_cfg->crop.top = grp_crop.y + grp_ctx->offset_top;
		grp_hw_cfg->crop.width = grp_crop.width;
		grp_hw_cfg->crop.height = grp_crop.height;
		TRACE_VPSS(DBG_DEBUG, "grp(%d) use frame crop.\n", grp_id);
	} else {
		// for grp crop.
		if (grp_ctx->grp_crop_info.enable) {
			grp_hw_cfg->crop.width = grp_ctx->grp_crop_info.crop_rect.width;
			grp_hw_cfg->crop.height = grp_ctx->grp_crop_info.crop_rect.height;
			grp_hw_cfg->crop.left = grp_ctx->grp_crop_info.crop_rect.x +
				grp_ctx->offset_left;
			grp_hw_cfg->crop.top = grp_ctx->grp_crop_info.crop_rect.y +
				grp_ctx->offset_top;
			TRACE_VPSS(DBG_DEBUG, "grp(%d) use GrpCrop.\n", grp_id);
		} else {
			grp_hw_cfg->crop.left = grp_ctx->offset_left;
			grp_hw_cfg->crop.top = grp_ctx->offset_top;
			grp_hw_cfg->crop.width = grp_ctx->grp_attr.w;
			grp_hw_cfg->crop.height = grp_ctx->grp_attr.h;
		}
	}
	osal_memcpy(&grp_hw_cfg->csc_cfg, &grp_ctx->csc, sizeof(grp_hw_cfg->csc_cfg));

	TRACE_VPSS(DBG_INFO, "grp(%d) Offset(left:%d top:%d right:%d bottom:%d) rect(%d %d %d %d)\n"
			, grp_id, grp_ctx->offset_left, grp_ctx->offset_top
			, grp_ctx->offset_right, grp_ctx->offset_bottom
			, grp_hw_cfg->crop.left, grp_hw_cfg->crop.top
			, grp_hw_cfg->crop.width, grp_hw_cfg->crop.height);

	TRACE_VPSS(DBG_DEBUG, "coef[0][0]: %#4x coef[0][1]: %#4x coef[0][2]: %#4x\n"
		, grp_hw_cfg->csc_cfg.coef[0][0]
		, grp_hw_cfg->csc_cfg.coef[0][1]
		, grp_hw_cfg->csc_cfg.coef[0][2]);
	TRACE_VPSS(DBG_DEBUG, "coef[1][0]: %#4x coef[1][1]: %#4x coef[1][2]: %#4x\n"
		, grp_hw_cfg->csc_cfg.coef[1][0]
		, grp_hw_cfg->csc_cfg.coef[1][1]
		, grp_hw_cfg->csc_cfg.coef[1][2]);
	TRACE_VPSS(DBG_DEBUG, "coef[2][0]: %#4x coef[2][1]: %#4x coef[2][2]: %#4x\n"
		, grp_hw_cfg->csc_cfg.coef[2][0]
		, grp_hw_cfg->csc_cfg.coef[2][1]
		, grp_hw_cfg->csc_cfg.coef[2][2]);
	TRACE_VPSS(DBG_DEBUG, "sub[0]: %3d sub[1]: %3d sub[2]: %3d\n"
		, grp_hw_cfg->csc_cfg.sub[0]
		, grp_hw_cfg->csc_cfg.sub[1]
		, grp_hw_cfg->csc_cfg.sub[2]);
	TRACE_VPSS(DBG_DEBUG, "add[0]: %3d add[1]: %3d add[2]: %3d\n"
		, grp_hw_cfg->csc_cfg.add[0]
		, grp_hw_cfg->csc_cfg.add[1]
		, grp_hw_cfg->csc_cfg.add[2]);

}

static int commit_hw_settings(struct vpss_grp_ctx *grp_ctx)
{
	vpss_grp grp_id = grp_ctx->grp_id;
	unsigned char online_from_isp = grp_ctx->online_from_isp;
	vpss_grp_attr_s *grp_attr = &grp_ctx->grp_attr;
	struct vpss_img_in_cfg *hw_grp_cfg = &grp_ctx->hw_cfg.grp_cfg;
	struct vpss_sc_cfg *hw_chn_cfg;
	struct vpss_chn_ctx *chn_ctx;
	struct vb_s *vb_in = NULL;
	vpss_chn chn_id;
	struct vb_jobs_t *jobs;
	unsigned char is_grp_changed = false;

	if (!online_from_isp && _is_frame_crop_changed(grp_id, grp_ctx))
		grp_ctx->is_cfg_changed = true; //DIS

	if (grp_ctx->is_cfg_changed) {
		_vpss_grp_hw_cfg_update(grp_ctx);
		grp_ctx->is_cfg_changed = false;
		is_grp_changed = true;
	}

	if (!online_from_isp) {
		if (!base_mod_jobs_waitq_empty(&grp_ctx->vb_jobs.ins)) {
			jobs = &grp_ctx->vb_jobs.ins;
			FIFO_GET_FRONT(&jobs->waitq, &vb_in);
		}
	}

	hw_grp_cfg->online_from_isp = online_from_isp;
	hw_grp_cfg->src_size.width = grp_attr->w;
	hw_grp_cfg->src_size.height = grp_attr->h;
	hw_grp_cfg->pixelformat = grp_attr->pixel_format;
	hw_grp_cfg->bytesperline[0] = (vb_in != NULL)
				? vb_in->buf.stride[0] : hw_grp_cfg->bytesperline[0];
	hw_grp_cfg->bytesperline[1] = (vb_in != NULL)
				? vb_in->buf.stride[1] : hw_grp_cfg->bytesperline[1];
	hw_grp_cfg->upsample = false;

	for (chn_id = 0; chn_id < grp_ctx->chn_max_num; ++chn_id) {
		chn_ctx = &grp_ctx->chn_ctxs[chn_id];
		hw_chn_cfg = &grp_ctx->hw_cfg.chn_cfg[chn_id];
		grp_ctx->hw_cfg.chn_enable[chn_id] = chn_ctx->is_enabled && (!chn_ctx->is_drop);

		if (!chn_ctx->is_enabled)
			continue;
		if (chn_ctx->is_drop)
			continue;

		if (is_grp_changed || chn_ctx->is_cfg_changed) {
			_vpss_chn_hw_cfg_update(chn_id, grp_ctx);
			chn_ctx->is_cfg_changed = false;
		}

		TRACE_VPSS(DBG_DEBUG, "grp(%d) chn(%d) size(%d %d) rect(%d %d %d %d)\n", grp_id, chn_id
				, chn_ctx->chn_attr.width, chn_ctx->chn_attr.height
				, hw_chn_cfg->crop.left, hw_chn_cfg->crop.top
				, hw_chn_cfg->crop.width, hw_chn_cfg->crop.height);

		hw_chn_cfg->pixelformat = chn_ctx->chn_attr.pixel_format;
		hw_chn_cfg->src_size.width = hw_grp_cfg->crop.width;
		hw_chn_cfg->src_size.height = hw_grp_cfg->crop.height;
		hw_chn_cfg->dst_size.width = chn_ctx->chn_attr.width;
		hw_chn_cfg->dst_size.height = chn_ctx->chn_attr.height;

		osal_memcpy(hw_chn_cfg->rgn_cfg, chn_ctx->rgn_cfg, sizeof(chn_ctx->rgn_cfg));

		hw_chn_cfg->rgn_coverex_cfg = chn_ctx->rgn_coverex_cfg;
		hw_chn_cfg->rgn_mosaic_cfg = chn_ctx->rgn_mosaic_cfg;

		if (chn_ctx->chn_attr.flip && chn_ctx->chn_attr.mirror)
			hw_chn_cfg->flip = SC_FLIP_HVFLIP;
		else if (chn_ctx->chn_attr.flip)
			hw_chn_cfg->flip = SC_FLIP_VFLIP;
		else if (chn_ctx->chn_attr.mirror)
			hw_chn_cfg->flip = SC_FLIP_HFLIP;
		else
			hw_chn_cfg->flip = SC_FLIP_NO;
		hw_chn_cfg->mute_cfg.enable = chn_ctx->is_muted;
		hw_chn_cfg->mute_cfg.color[0] = 0;
		hw_chn_cfg->mute_cfg.color[1] = 0;
		hw_chn_cfg->mute_cfg.color[2] = 0;

		hw_chn_cfg->sb_cfg.sb_mode = chn_ctx->buf_wrap.enable ? chn_ctx->sbm_ctx.sb_mode : 0;
		hw_chn_cfg->sb_cfg.sb_size = (chn_ctx->buf_wrap.buf_line == SB_LINE_64) ? 0 : 1;
		hw_chn_cfg->sb_cfg.sb_nb = chn_ctx->buf_wrap.wrap_buffer_size;
		hw_chn_cfg->sb_cfg.sb_full_nb = chn_ctx->buf_wrap.wrap_buffer_size;
		hw_chn_cfg->sb_cfg.sb_wr_ctrl_idx = 0;

		if (VPSS_UPSAMPLE(grp_attr->pixel_format, chn_ctx->chn_attr.pixel_format)
			&& (!grp_ctx->is_copy_upsample))
			hw_grp_cfg->upsample = true;
	}

	return 0;
}

/* _vpss_chl_frame_rate_ctrl: dynamically disabled chn per frame-rate-ctrl
 *
 * @param proc_ctx: the frame statics for reference
 * @param ctx: the working settings
 */
static int simplify_rate(unsigned int dst_in, unsigned int src_in, unsigned int *dst_out, unsigned int *src_out)
{
	unsigned int i = 1;
	unsigned int a, b;

	while (i < dst_in + 1) {
		a = dst_in % i;
		b = src_in % i;
		if (a == 0 && b == 0) {
			dst_in = dst_in / i;
			src_in = src_in / i;
			i = 1;
		}
		i++;
	}
	*dst_out = dst_in;
	*src_out = src_in;
	return 0;
}

static unsigned char vpss_frame_ctrl(unsigned long long frame_index, frame_rate_ctrl_s *frame_rate)
{
	unsigned int src_simp;
	unsigned int dst_simp;
	unsigned int index;
	unsigned int src_duration, dst_duration;
	unsigned int cur_index, next_index;

	simplify_rate(frame_rate->dst_frame_rate, frame_rate->src_frame_rate,
		&dst_simp, &src_simp);

	index = osal_div_u64(frame_index, src_simp);
	if (index == 0) {
		return true;
	}
	src_duration = 100;
	dst_duration = (src_duration * src_simp) / dst_simp;
	cur_index = (index - 1) * src_duration / dst_duration;
	next_index = index * src_duration / dst_duration;

	if (next_index == cur_index)
		return false;

	return true;
}

static unsigned char _vpss_chl_frame_rate_ctrl(struct vpss_grp_ctx *grp_ctx, unsigned char working_mask)
{
	vpss_chn chn_id;

	for (chn_id = 0; chn_id < grp_ctx->chn_max_num; ++chn_id) {
		if (!grp_ctx->chn_ctxs[chn_id].is_enabled || !grp_ctx->chn_ctxs[chn_id].chn_work_status.send_ok)
			continue;
		grp_ctx->chn_ctxs[chn_id].is_drop = false;
		if (FRC_INVALID(grp_ctx->chn_ctxs[chn_id].chn_attr.frame_rate))
			continue;
		if (!vpss_frame_ctrl(grp_ctx->grp_work_status.frc_recv_cnt - 1,
			&grp_ctx->chn_ctxs[chn_id].chn_attr.frame_rate)) {
			grp_ctx->chn_ctxs[chn_id].is_drop = true;
			working_mask &= ~BIT(chn_id);
			TRACE_VPSS(DBG_DEBUG, "chn[%d] frame index(%d) drop\n", chn_id,
				grp_ctx->grp_work_status.frc_recv_cnt);
		}
	}

	if (!(working_mask & 0xf))
		working_mask = 0;

	return working_mask;
}

static void _update_vpss_grp_proc(unsigned int duration,
		unsigned int hw_duration, struct vpss_grp_work_status_s *grp_status)
{
	grp_status->cost_time = duration;
	if (grp_status->max_cost_time < grp_status->cost_time)
		grp_status->max_cost_time = grp_status->cost_time;

	grp_status->hw_cost_time = hw_duration;
	if (grp_status->hw_max_cost_time < grp_status->hw_cost_time)
		grp_status->hw_max_cost_time = grp_status->hw_cost_time;
}

static void _update_vpss_chn_proc(struct vpss_chn_work_status_s *chn_status)
{
	chn_status->send_ok++;
	chn_status->frame_num++;
}

static unsigned char _vpss_check_gdc_job(mmf_chn_s chn, vb_blk blk, struct vpss_chn_ctx *chn_ctx)
{
	struct gdc_mesh *pmesh;

	pmesh = &chn_ctx->mesh;
	if (!osal_atomic_cmpxchg(&pmesh->gdc_flag, 0, 1)) {
		if (chn_ctx->ldc_attr.enable) {
			struct vb_s *vb = (struct vb_s *)(uintptr_t)blk;
			struct _vpss_gdc_cb_param cb_param = { .chn = chn, .usage = GDC_USAGE_LDC};

			if (_mesh_gdc_do_op_cb(GDC_USAGE_LDC
				, &chn_ctx->ldc_attr.attr
				, vb
				, chn_ctx->chn_attr.pixel_format
				, pmesh->paddr
				, false, &cb_param
				, sizeof(cb_param)
				, ID_VPSS
				, chn_ctx->ldc_attr.attr.rotation) != 0) {
				osal_atomic_set(&pmesh->gdc_flag, 0);
				TRACE_VPSS(DBG_ERR, "gdc LDC failed.\n");

				// GDC failed, pass buffer to next module, not block here
				//   e.g. base_get_chn_buffer(-1) blocking
				return false;
			}
			return true;
		} else if (chn_ctx->rotation != ROTATION_0) {
			struct vb_s *vb = (struct vb_s *)(uintptr_t)blk;
			struct _vpss_gdc_cb_param cb_param = { .chn = chn,
				.usage = GDC_USAGE_ROTATION };

			if (_mesh_gdc_do_op_cb(GDC_USAGE_ROTATION
				, NULL
				, vb
				, chn_ctx->chn_attr.pixel_format
				, pmesh->paddr
				, false, &cb_param
				, sizeof(cb_param)
				, ID_VPSS
				, chn_ctx->rotation) != 0) {
				osal_atomic_set(&pmesh->gdc_flag, 0);
				TRACE_VPSS(DBG_ERR, "gdc rotation failed.\n");

				// GDC failed, pass buffer to next module, not block here
				//   e.g. base_get_chn_buffer(-1) blocking
				return false;
			}
			return true;
		}
		osal_atomic_set(&pmesh->gdc_flag, 0);
	} else {
		TRACE_VPSS(DBG_WARN, "grp(%d) chn(%d) drop frame due to gdc op blocked.\n",
			chn.dev_id, chn.chn_id);
		// release blk if gdc not done yet
		vb_release_block(blk);
		return true;
	}

	return false;
}

void vpss_notify_wkup_evt(struct vpss_handler_ctx *hdl)
{
	osal_spin_lock(&hdl->hdl_lock);
	hdl->events |= CTX_EVENT_WKUP;
	osal_spin_unlock(&hdl->hdl_lock);

	osal_wait_wakeup(&hdl->wait);
}

int vpss_online_prepare(int working_grp, struct vpss_ctx *ctx)
{
	unsigned char i, working_mask = 0;
	struct vpss_job *past_job[VPSS_ONLINE_JOB_NUM] = {[0 ... VPSS_ONLINE_JOB_NUM - 1] = NULL};
	struct vpss_grp_ctx *grp_ctx = ctx->grp_ctx[working_grp];

	osal_mutex_lock(&grp_ctx->lock);

	// sc's mask
	working_mask = get_work_mask(grp_ctx);
	if (working_mask == 0) {
		TRACE_VPSS(DBG_WARN, "grp(%d) none of the channels are enable.\n", working_grp);
		goto err;
	}

	// commit hw settings of this vpss-grp.
	if (commit_hw_settings(grp_ctx) != 0) {
		TRACE_VPSS(DBG_ERR, "grp(%d) apply hw settings NG.\n", working_grp);
		grp_ctx->grp_work_status.start_fail_cnt++;
		goto err;
	}

	for (i = 0; i < VPSS_ONLINE_JOB_NUM; i++) {
		if (FIFO_EMPTY(&grp_ctx->jobq)) {
			TRACE_VPSS(DBG_ERR, "vpss(%d) jobq empty.\n", working_grp);
			goto err;
		}
		FIFO_POP(&grp_ctx->jobq, &past_job[i]);
		osal_memcpy(&past_job[i]->cfg, &grp_ctx->hw_cfg, sizeof(grp_ctx->hw_cfg));

		if (fill_buffers(grp_ctx, past_job[i]) != 0) {
			TRACE_VPSS(DBG_ERR, "grp(%d) fill buffer NG.\n", working_grp);
			grp_ctx->grp_work_status.start_fail_cnt++;
			goto err;
		}
	}

	osal_gettimeofday(&grp_ctx->time);
	osal_atomic_set(&grp_ctx->hdl_state, HANDLER_STATE_RUN);
	for (i = 0; i < VPSS_ONLINE_JOB_NUM; i++)
		vpss_hal_push_online_job(past_job[i], (struct vpss_hal_ctx *)grp_ctx->hal_ctx_ptr);
	osal_mutex_unlock(&grp_ctx->lock);

	TRACE_VPSS(DBG_DEBUG, "Online Grp(%d) post job.\n", working_grp);

	return 0;

err:
	for (i = 0; i < VPSS_ONLINE_JOB_NUM; i++)
		if (past_job[i])
			FIFO_PUSH(&grp_ctx->jobq, past_job[i]);
	osal_mutex_unlock(&grp_ctx->lock);

	return -1;
}

static int vpss_online_full_job(vpss_grp working_grp, struct vpss_job *job, struct vpss_grp_ctx *grp_ctx)
{
	unsigned char working_mask = 0;

	// sc's mask
	working_mask = get_work_mask(grp_ctx);
	if (working_mask)
		working_mask = _vpss_chl_frame_rate_ctrl(grp_ctx, working_mask);
	if (working_mask == 0) {
		TRACE_VPSS(DBG_WARN, "grp(%d) none of the channels are enable.\n", working_grp);
		goto err;
	}

	// commit hw settings of this vpss-grp.
	if (commit_hw_settings(grp_ctx) != 0) {
		TRACE_VPSS(DBG_ERR, "grp(%d) apply hw settings NG.\n", working_grp);
		grp_ctx->grp_work_status.start_fail_cnt++;
		goto err;
	}

	osal_memcpy(&job->cfg, &grp_ctx->hw_cfg, sizeof(grp_ctx->hw_cfg));

	if (fill_buffers(grp_ctx, job) != 0) {
		TRACE_VPSS(DBG_ERR, "grp(%d) fill buffer NG.\n", working_grp);
		grp_ctx->grp_work_status.start_fail_cnt++;
		goto err;
	}

	osal_gettimeofday(&grp_ctx->time);
	osal_atomic_set(&grp_ctx->hdl_state, HANDLER_STATE_RUN);

	TRACE_VPSS(DBG_DEBUG, "Online Grp(%d) post job.\n", working_grp);

	return 0;

err:
	return -1;
}

void vpss_handle_online_frame_done(struct vpss_job *job)
{
	struct vpss_grp_ctx *grp_ctx = (struct vpss_grp_ctx *)job->data;
	vpss_grp working_grp = job->grp_id;
	mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = working_grp, .chn_id = 0};
	vb_blk blk;
	struct vb_s *vb;
	vpss_chn chn_id;
	unsigned int duration;
	osal_timeval time;

	TRACE_VPSS(DBG_INFO, "grp(%d) eof\n", working_grp);

	osal_mutex_lock(&grp_ctx->lock);
	grp_ctx->grp_work_status.recv_cnt++;
	grp_ctx->grp_work_status.frc_recv_cnt++;
	osal_gettimeofday(&time);
	duration = get_diff_in_us(grp_ctx->time, time);

	chn_id = 0;
	do {
		if (!grp_ctx->chn_ctxs[chn_id].is_enabled)
			continue;
		if (!job->cfg.chn_enable[chn_id])
			continue;
		if (grp_ctx->chn_ctxs[chn_id].buf_wrap.enable) {
			_update_vpss_chn_proc(&grp_ctx->chn_ctxs[chn_id].chn_work_status);
			continue;
		}

		chn.chn_id = chn_id;

		vb_dqbuf(chn, &grp_ctx->vb_jobs.outs[chn_id], &blk);
		if (blk == VB_INVALID_HANDLE) {
			TRACE_VPSS(DBG_ERR, "Mod(%d) can't get vb-blk.\n", chn.mod_id);
			continue;
		}
		//if (vi_ctx.bypass_frm[working_grp] >= vpssPrcCtx[working_grp].chn_cfgs[chn_id].send_ok) {
		//	TRACE_VPSS(DBG_DEBUG, "grp(%d) chn(%d) drop frame for vi-bypass(%d).\n",
		//		       working_grp, chn_id, vi_ctx.bypass_frm[working_grp]);
		//	vb_release_block(blk);
		//	_update_vpss_chn_proc(working_grp, chn_id);
		//	continue;
		//}

		// update pts & frm_num info to vb
		vb = (struct vb_s *)(uintptr_t)blk;
		if (vb->buf.frame_flag == 1) {
			TRACE_VPSS(DBG_INFO, "grp(%d) chn(%d) drop frame.\n", working_grp, chn_id);
			vb_release_block(blk);
			continue;
		}

		vb->buf.pts = job->online_param.ts.tv_sec * 1000000 + job->online_param.ts.tv_usec;
		vb->buf.dev_num = working_grp;
		vb->buf.frame_flag = job->info.checksum[chn_id];
		vb->buf.frm_num = job->online_param.frm_num;

		if (_vpss_check_gdc_job(chn, blk, &grp_ctx->chn_ctxs[chn_id]) != true)
			vb_done_handler(chn, CHN_TYPE_OUT, &grp_ctx->vb_jobs.outs[chn_id], blk);

		TRACE_VPSS(DBG_INFO, "grp(%d) chn(%d) end\n", working_grp, chn_id);
		_update_vpss_chn_proc(&grp_ctx->chn_ctxs[chn_id].chn_work_status);
	} while (++chn_id < VPSS_MAX_CHN_NUM);

	// Update vpss grp proc info
	_update_vpss_grp_proc(duration, job->info.hw_duration, &grp_ctx->grp_work_status);

	if (!vpss_online_full_job(working_grp, job, grp_ctx))
		vpss_hal_push_online_job(job, (struct vpss_hal_ctx *)grp_ctx->hal_ctx_ptr);

	osal_mutex_unlock(&grp_ctx->lock);
}


static void vpss_handle_offline_frame_done(struct vpss_job *job)
{
	struct vpss_grp_ctx *grp_ctx = (struct vpss_grp_ctx *)job->data;
	vpss_grp working_grp = job->grp_id;
	mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = working_grp, .chn_id = 0};
	vb_blk blk;
	struct vb_s *vb;
	vpss_chn chn_id;
	unsigned int duration;
	osal_timeval time;

	TRACE_VPSS(DBG_INFO, "grp(%d) eof\n", working_grp);

	osal_mutex_lock(&grp_ctx->lock);

	//Todo: osal_spin_lock, vpss destroy?
	vb_dqbuf(chn, &grp_ctx->vb_jobs.ins, &blk);
	if (blk == VB_INVALID_HANDLE) {
		TRACE_VPSS(DBG_ERR, "Mod(%d) can't get vb-blk.\n", chn.mod_id);
	} else {
		vb_done_handler(chn, CHN_TYPE_IN, &grp_ctx->vb_jobs.ins, blk);
	}

	chn_id = 0;
	do {
		if (!grp_ctx->chn_ctxs[chn_id].is_enabled)
			continue;
		if (!job->cfg.chn_enable[chn_id])
			continue;
		if (grp_ctx->chn_ctxs[chn_id].buf_wrap.enable) {
			_update_vpss_chn_proc(&grp_ctx->chn_ctxs[chn_id].chn_work_status);
			continue;
		}

		chn.chn_id = chn_id;

		vb_dqbuf(chn, &grp_ctx->vb_jobs.outs[chn_id], &blk);
		if (blk == VB_INVALID_HANDLE) {
			TRACE_VPSS(DBG_ERR, "Mod(%d) can't get vb-blk.\n", chn.mod_id);
			continue;
		}
		vb = (struct vb_s *)(uintptr_t)blk;
		if (vb->buf.frame_flag == 1) {
			TRACE_VPSS(DBG_INFO, "grp(%d) chn(%d) drop frame.\n", working_grp, chn_id);
			vb_release_block(blk);
			continue;
		}

		vb->buf.frame_flag = job->info.checksum[chn_id];
		if (_vpss_check_gdc_job(chn, blk, &grp_ctx->chn_ctxs[chn_id]) != true)
			vb_done_handler(chn, CHN_TYPE_OUT, &grp_ctx->vb_jobs.outs[chn_id], blk);

		TRACE_VPSS(DBG_INFO, "grp(%d) chn(%d) end\n", working_grp, chn_id);
		_update_vpss_chn_proc(&grp_ctx->chn_ctxs[chn_id].chn_work_status);
	} while (++chn_id < VPSS_MAX_CHN_NUM);

	osal_gettimeofday(&time);
	duration = get_diff_in_us(grp_ctx->time, time);

	// Update vpss grp proc info
	_update_vpss_grp_proc(duration, job->info.hw_duration, &grp_ctx->grp_work_status);
	FIFO_PUSH(&grp_ctx->jobq, job);
	osal_atomic_set(&grp_ctx->hdl_state, HANDLER_STATE_STOP);
	osal_mutex_unlock(&grp_ctx->lock);
}

void vpss_handle_frame_done(osal_workqueue *work)
{
	struct vpss_job *job = osal_container_of(work, struct vpss_job, work);
	struct vpss_grp_ctx *grp_ctx = (struct vpss_grp_ctx *)job->data;

	if (!grp_ctx->is_created || !grp_ctx->is_started) {
		TRACE_VPSS(DBG_NOTICE, "Grp(%d) isn't start yet.\n", grp_ctx->grp_id);
		return;
	}

	if (grp_ctx->online_from_isp) {
		vpss_handle_online_frame_done(job);
	} else {
		vpss_handle_offline_frame_done(job);
		vpss_notify_wkup_evt((struct vpss_handler_ctx *)grp_ctx->hdl_ctx_ptr);
	}
}

void vpss_wkup_frame_done_handle(void *pdata)
{
	struct vpss_job *job = (struct vpss_job *)pdata;

	osal_workqueue_schedule_highpri(&job->work);
}

/**
 * @return: 0 if ready
 */
static int vpss_try_schedule(int working_grp, struct vpss_ctx *ctx)
{
	int i;
	unsigned char working_mask = 0;
	struct vpss_grp_ctx *grp_ctx = ctx->grp_ctx[working_grp];
	struct vpss_job *job = NULL;
	mmf_chn_s chn = {.mod_id = ID_VPSS, .dev_id = working_grp, .chn_id = 0};

	osal_mutex_lock(&grp_ctx->lock);
	if (!grp_ctx->is_started)
		goto vpss_next_job;

	if (FIFO_EMPTY(&grp_ctx->jobq)) {
		TRACE_VPSS(DBG_ERR, "vpss(%d) jobq empty.\n", working_grp);
		goto vpss_next_job;
	}

	// sc's mask
	working_mask = get_work_mask(grp_ctx);
	if (working_mask)
		working_mask = _vpss_chl_frame_rate_ctrl(grp_ctx, working_mask);
	if (working_mask == 0) {
		TRACE_VPSS(DBG_NOTICE, "grp(%d) working_mask zero.\n", working_grp);
		_release_vpss_waitq(chn, CHN_TYPE_IN, grp_ctx);
		goto vpss_next_job;
	}

	// commit hw settings of this vpss-grp.
	if (commit_hw_settings(grp_ctx) != 0) {
		TRACE_VPSS(DBG_ERR, "grp(%d) apply hw settings NG.\n", working_grp);
		_release_vpss_waitq(chn, CHN_TYPE_IN, grp_ctx);
		grp_ctx->grp_work_status.start_fail_cnt++;
		goto vpss_next_job;
	}
	FIFO_POP(&grp_ctx->jobq, &job);
	osal_memcpy(&job->cfg, &grp_ctx->hw_cfg, sizeof(grp_ctx->hw_cfg));

	if (fill_buffers(grp_ctx, job) != 0) {
		TRACE_VPSS(DBG_ERR, "grp(%d) fill buffer NG.\n", working_grp);
		grp_ctx->grp_work_status.start_fail_cnt++;
		FIFO_PUSH(&grp_ctx->jobq, job);
		goto vpss_next_job;
	}

	osal_gettimeofday(&grp_ctx->time);
	osal_atomic_set(&grp_ctx->hdl_state, HANDLER_STATE_RUN);
	osal_mutex_unlock(&grp_ctx->lock);

	if (vpss_hal_push_job(job, (struct vpss_hal_ctx *)grp_ctx->hal_ctx_ptr)) {
		osal_atomic_set(&grp_ctx->hdl_state, HANDLER_STATE_STOP);
		FIFO_PUSH(&grp_ctx->jobq, job);
		release_buffers(grp_ctx);
		return -1;
	}

	TRACE_VPSS(DBG_INFO, "Offline Grp(%d) post job.\n", working_grp);
	vpss_hal_try_schedule((struct vpss_hal_ctx *)grp_ctx->hal_ctx_ptr);

	return 0;

vpss_next_job:
	grp_ctx->is_cfg_changed = true;
	for (i = 0; i < grp_ctx->chn_max_num; i++)
		grp_ctx->chn_ctxs[i].is_cfg_changed = true;
	osal_mutex_unlock(&grp_ctx->lock);

	return -1;
}


static unsigned char vpss_handler_is_idle(struct vpss_ctx *ctx)
{
	int i;

	for (i = 0; i < VPSS_MAX_GRP_NUM; i++)
		if (ctx->grp_ctx[i] && ctx->grp_ctx[i]->is_created && ctx->grp_ctx[i]->is_started)
			return false;

	return true;
}

int vpss_wait_cond_func(const void *param)
{
	struct vpss_handler_ctx *hdl_ctx = (struct vpss_handler_ctx *)param;

	return hdl_ctx->events;
}

static int vpss_event_handler(void *arg)
{
	struct vpss_ctx *ctx = (struct vpss_ctx *)arg;
	struct vpss_handler_ctx *hdl_ctx = &ctx->hdl_ctx;
	struct vpss_grp_ctx *grp_ctx;
	unsigned long idle_timeout = IDLE_TIMEOUT_MS;
	unsigned long eof_timeout = EOF_WAIT_TIMEOUT_MS;
	unsigned long timeout = idle_timeout;
	int i, ret;
	int grp = 0, prev_grp = -1;
	struct vb_jobs_t *jobs;

	while (!hdl_ctx->stop_flag) {
		ret = osal_wait_timeout_interruptible(&hdl_ctx->wait, vpss_wait_cond_func,
			hdl_ctx, timeout);

		/* -%ERESTARTSYS */
		if (ret < 0)
			break;

		/* timeout */
		if (!ret && vpss_handler_is_idle(ctx)) {
			timeout = idle_timeout;
			continue;
		}

		//TRACE_VPSS(DBG_DEBUG, "vpss thread, events:%d\n", ctx->events);
		osal_spin_lock(&hdl_ctx->hdl_lock);
		hdl_ctx->events &= ~CTX_EVENT_WKUP;
		osal_spin_unlock(&hdl_ctx->hdl_lock);

		if (osal_atomic_read(&hdl_ctx->active_cnt) == 0)
			continue;

		grp = prev_grp;
		for (i = 0; i < VPSS_MAX_GRP_NUM; i++) {
			if (++grp >= VPSS_MAX_GRP_NUM)
				grp = 0;

			grp_ctx = ctx->grp_ctx[grp];
			if (!grp_ctx || !grp_ctx->is_started)
				continue;
			if (osal_atomic_read(&grp_ctx->hdl_state) == HANDLER_STATE_RUN) {
				//if (get_diff_in_us(vpss_ctx[grp]->time, time) > (eof_timeout * 1000))
				//	vpss_timeout(vpss_ctx[grp]);
				continue;
			}
			jobs = &grp_ctx->vb_jobs.ins;
			if (!jobs) {
				TRACE_VPSS(DBG_INFO, "get jobs failed\n");
				continue;
			}

			if (!osal_sem_trydown(&jobs->sem)) {
				vpss_try_schedule(grp, ctx);
				timeout = eof_timeout;
				prev_grp = grp;
			}
		}
	}

	return 0;
}

int vpss_grp_qbuf(mmf_chn_s chn, vb_blk blk, void *data)
{
	int ret;
	struct vpss_ctx *ctx = (struct vpss_ctx *)data;
	vpss_grp grp_id = chn.dev_id;
	struct vpss_grp_ctx *grp_ctx;

	ret = check_vpss_grp_valid(grp_id);
	if (ret != 0)
		return ret;

	ret = check_vpss_grp_created(grp_id, ctx);
	if (ret != 0)
		return ret;

	grp_ctx = ctx->grp_ctx[grp_id];

	if (!grp_ctx->is_started) {
		TRACE_VPSS(DBG_NOTICE, "Grp(%d) not started yet.\n", grp_id);
		return -1;
	}
	if (grp_ctx->online_from_isp) {
		TRACE_VPSS(DBG_ERR, "Grp(%d) online, No need to receive buffer.\n", grp_id);
		return -1;
	}
	grp_ctx->grp_work_status.recv_cnt++;

	if (!FRC_INVALID(grp_ctx->grp_attr.frame_rate) &&
		!vpss_frame_ctrl(grp_ctx->grp_work_status.recv_cnt - 1,
		&grp_ctx->grp_attr.frame_rate)) {
		TRACE_VPSS(DBG_DEBUG, "grp[%d] frame index(%d) drop\n", grp_id,
			grp_ctx->grp_work_status.recv_cnt);
		return 0;
	}
	grp_ctx->grp_work_status.frc_recv_cnt++;

	TRACE_VPSS(DBG_INFO, "Grp(%d) qbuf, blk(%llx)\n", grp_id, blk);

	ret = vb_qbuf(chn, CHN_TYPE_IN, &grp_ctx->vb_jobs.ins, blk);
	if (ret != 0) {
		grp_ctx->grp_work_status.lost_cnt++;
		return ret;
	}

	vpss_notify_wkup_evt(&ctx->hdl_ctx);

	return 0;
}

void _update_vpss_chn_real_frame_rate(struct vpss_ctx *ctx, unsigned int duration_us)
{
	int i, j;
	struct vpss_chn_ctx *chn_ctx;


	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		if (ctx->grp_ctx[i] && ctx->grp_ctx[i]->is_created) {
			for (j = 0; j < VPSS_MAX_CHN_NUM; ++j) {
				chn_ctx = &ctx->grp_ctx[i]->chn_ctxs[j];

				if (chn_ctx->is_enabled) {
					chn_ctx->chn_work_status.real_frame_rate
						= chn_ctx->chn_work_status.frame_num * duration_us / 1000000;
					chn_ctx->chn_work_status.frame_num = 0;
				}
			}
		}
	}
}

void vpss_ctx_param_init(struct vpss_ctx *ctx)
{
	int i;

	osal_mutex_lock(&ctx->lock);
	for (i = 0; i < VPSS_MAX_GRP_NUM; ++i) {
		ctx->grp_used[i] = false;
	}
	ctx->sb_phy_addr = 0;
	ctx->sb_width = 0;
	ctx->sb_height = 0;
	ctx->sb_buf_line = 0;
	ctx->sb_buffer_size = 0;
	osal_mutex_unlock(&ctx->lock);
}

void vpss_init(struct vpss_ctx *ctx)
{
	struct vbq_recv_s vpss_cb;
	struct vpss_handler_ctx *hdl_ctx = &ctx->hdl_ctx;

	vpss_cb.cb = vpss_grp_qbuf;
	vpss_cb.data = ctx;
	base_register_recv_cb(ID_VPSS, &vpss_cb);

	osal_mutex_init(&ctx->lock);
	vpss_ctx_param_init(ctx);

	//init handler
	osal_wait_init(&hdl_ctx->wait);
	osal_spin_lock_init(&hdl_ctx->hdl_lock);
	osal_atomic_set(&hdl_ctx->active_cnt, 0);
	hdl_ctx->events = 0;
	hdl_ctx->stop_flag = 0;

	hdl_ctx->thread = osal_kthread_create(vpss_event_handler, ctx, "task_vpss_hdl", 0);
	if (!hdl_ctx->thread) {
		TRACE_VPSS(DBG_ERR, "vpss kthread_create failed.\n");
		return;
	}
	osal_kthread_set_priority(hdl_ctx->thread, OSAL_TASK_PRIORITY_HIGH);
}

void vpss_deinit(struct vpss_ctx *ctx)
{
	struct vpss_handler_ctx *hdl_ctx = &ctx->hdl_ctx;

	hdl_ctx->stop_flag = 1;
	osal_kthread_destroy(hdl_ctx->thread, hdl_ctx->stop_flag);
	hdl_ctx->thread = NULL;
	osal_spin_lock_destroy(&hdl_ctx->hdl_lock);
	osal_wait_destroy(&hdl_ctx->wait);

	osal_mutex_destroy(&ctx->lock);
	base_unregister_recv_cb(ID_VPSS);
}

