/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_vip_core.c
 * Description: video pipeline core driver
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/of_reserved_mem.h>
#include <linux/videodev2.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/version.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-common.h>

#include "vip/vip_common.h"
#include "vip/scaler.h"
#include "vip/dsi_phy.h"
#include "vip/dwa.h"
#include "uapi/isp_reg.h"
#include "vip/isp_drv.h"

#include "cvi_debug.h"
#include "cvi_vip_core.h"
#include "cvi_vip_disp.h"
#include "cvi_vip_sc.h"
#include "cvi_vip_img.h"
#include "cvi_vip_dwa.h"
#include "cvi_vip_isp.h"
#include "cvi_vip_vpss_proc.h"
#include "cvi_vip_rgn_proc.h"

int dump_reg = 1;
int log_level = VIP_ERR | VIP_INFO;
int debug;
int vip_clk_freq;

const struct cvi_vip_fmt cvi_vip_formats[] = {
	{
	.fourcc      = V4L2_PIX_FMT_YUV420M,
	.fmt         = SCL_FMT_YUV420,
	.bit_depth   = { 8, 4, 4 },
	.buffers     = 3,
	.plane_sub_h = 2,
	.plane_sub_v = 2,
	},
	{
	.fourcc      = V4L2_PIX_FMT_YUV422M,
	.fmt         = SCL_FMT_YUV422,
	.bit_depth   = { 8, 4, 4 },
	.buffers     = 3,
	.plane_sub_h = 2,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = V4L2_PIX_FMT_YUV444M,
	.fmt         = SCL_FMT_RGB_PLANAR,
	.bit_depth   = { 8, 8, 8 },
	.buffers     = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = V4L2_PIX_FMT_RGBM, /* rgb */
	.fmt         = SCL_FMT_RGB_PLANAR,
	.bit_depth   = { 8, 8, 8 },
	.buffers     = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = V4L2_PIX_FMT_RGB24, /* rgb */
	.fmt         = SCL_FMT_RGB_PACKED,
	.bit_depth   = { 24 },
	.buffers     = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = V4L2_PIX_FMT_BGR24, /* bgr */
	.fmt         = SCL_FMT_BGR_PACKED,
	.bit_depth   = { 24 },
	.buffers     = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = V4L2_PIX_FMT_GREY, /* Y-Only */
	.fmt         = SCL_FMT_Y_ONLY,
	.bit_depth   = { 8 },
	.buffers     = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = V4L2_PIX_FMT_HSV24, /* hsv */
	.fmt         = SCL_FMT_RGB_PACKED,
	.bit_depth   = { 24 },
	.buffers     = 1,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
	{
	.fourcc      = V4L2_PIX_FMT_HSVM, /* hsv */
	.fmt         = SCL_FMT_RGB_PLANAR,
	.bit_depth   = { 8, 8, 8 },
	.buffers     = 3,
	.plane_sub_h = 1,
	.plane_sub_v = 1,
	},
};

module_param(log_level, int, 0644);
/* debug: for tile mode debug
 * - bit[0]: if true, sc force tile mode
 * - bit[1]: if true, sc only do left tile.
 * - bit[2]: if true, disable ccf on sc_top/vip_sys1/sc/img/disp/dwa
 */
module_param(debug, int, 0644);

module_param(vip_clk_freq, int, 0644);

char *v4l2_fourcc2s(u32 fourcc, char *buf)
{
	buf[0] = fourcc & 0x7f;
	buf[1] = (fourcc >> 8) & 0x7f;
	buf[2] = (fourcc >> 16) & 0x7f;
	buf[3] = (fourcc >> 24) & 0x7f;
	buf[4] = '\0';
	return buf;
}

/*************************************************************************
 *	General functions
 *************************************************************************/
const struct cvi_vip_fmt *cvi_vip_get_format(u32 pixelformat)
{
	const struct cvi_vip_fmt *fmt;
	unsigned int k;

	for (k = 0; k < ARRAY_SIZE(cvi_vip_formats); k++) {
		fmt = &cvi_vip_formats[k];
		if (fmt->fourcc == pixelformat)
			return fmt;
	}

	return NULL;
}

int cvi_vip_enum_fmt_vid(struct file *file, void *priv, struct v4l2_fmtdesc *f)
{
	const struct cvi_vip_fmt *fmt;

	if (f->index >= ARRAY_SIZE(cvi_vip_formats))
		return -EINVAL;

	fmt = &cvi_vip_formats[f->index];
	f->pixelformat = fmt->fourcc;

	return 0;
}

int cvi_vip_try_fmt_vid_mplane(struct v4l2_format *f, u8 align)
{
	struct v4l2_pix_format_mplane *mp = &f->fmt.pix_mp;
	struct v4l2_plane_pix_format *pfmt = mp->plane_fmt;
	const struct cvi_vip_fmt *fmt;
	unsigned int bytesperline;
	u8 p;
	char buf[5];

	dprintk(VIP_DBG, "+\n");
	fmt = cvi_vip_get_format(mp->pixelformat);
	dprintk(VIP_INFO, "size(%d-%d) fourcc(%s)\n", mp->width, mp->height,
		v4l2_fourcc2s(mp->pixelformat, buf));
	if (!fmt) {
		dprintk(VIP_ERR, "fourcc(%s) unknown.\n",
			v4l2_fourcc2s(mp->pixelformat, buf));
		return -EINVAL;
	}

	if (align < VIP_ALIGNMENT)
		align = VIP_ALIGNMENT;

	mp->field = V4L2_FIELD_NONE;    // progressive only
	mp->width = clamp_val(mp->width, SCL_MIN_WIDTH, SCL_MAX_WIDTH * 2);
	if (mp->pixelformat == V4L2_PIX_FMT_YUV420M)
		mp->height = ALIGN(mp->height, 2);
	if (IS_YUV_FMT(fmt->fmt)) {
		// YUV422/420
		mp->width &= ~(fmt->plane_sub_h - 1);
		mp->height &= ~(fmt->plane_sub_v - 1);
	}

	mp->num_planes = fmt->buffers;
	if (mp->pixelformat == V4L2_PIX_FMT_YUV420M) {
		/* maks Y stride as twice of CS stride if it is YUV420 */
		bytesperline = ALIGN((mp->width * fmt->bit_depth[1]) >> 3, align);
		if (pfmt[1].bytesperline < bytesperline)
			pfmt[1].bytesperline = bytesperline;
		if (pfmt[2].bytesperline < bytesperline)
			pfmt[2].bytesperline = bytesperline;
		bytesperline = bytesperline << 1;
		if (pfmt[0].bytesperline < bytesperline)
			pfmt[0].bytesperline = bytesperline;
		pfmt[0].sizeimage = pfmt[0].bytesperline * mp->height / 1;
		pfmt[1].sizeimage = pfmt[1].bytesperline * mp->height / fmt->plane_sub_v;
		pfmt[2].sizeimage = pfmt[2].bytesperline * mp->height / fmt->plane_sub_v;

		memset(pfmt[0].reserved, 0, sizeof(pfmt[0].reserved));
		memset(pfmt[1].reserved, 0, sizeof(pfmt[1].reserved));
		memset(pfmt[2].reserved, 0, sizeof(pfmt[2].reserved));
	} else  {
		for (p = 0; p < mp->num_planes; p++) {
			u8 plane_sub_v = (p == 0) ? 1 : fmt->plane_sub_v;
			/* Calculate the minimum supported bytesperline value */
			bytesperline = ALIGN((mp->width * fmt->bit_depth[p]) >> 3, align);

			if (pfmt[p].bytesperline < bytesperline)
				pfmt[p].bytesperline = bytesperline;

			pfmt[p].sizeimage = pfmt[p].bytesperline * mp->height / plane_sub_v;

			dprintk(VIP_INFO, "plane-%d: bytesperline(%d) sizeimage(%x)\n", p,
				pfmt[p].bytesperline, pfmt[p].sizeimage);
			memset(pfmt[p].reserved, 0, sizeof(pfmt[p].reserved));
		}
	}

	mp->xfer_func = V4L2_XFER_FUNC_DEFAULT;
	mp->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	mp->quantization = V4L2_QUANTIZATION_DEFAULT;
	if (mp->pixelformat == V4L2_PIX_FMT_RGBM ||
	    mp->pixelformat == V4L2_PIX_FMT_RGB24 ||
	    mp->pixelformat == V4L2_PIX_FMT_BGR24 ||
		mp->pixelformat == V4L2_PIX_FMT_GREY) {
		mp->colorspace = V4L2_COLORSPACE_SRGB;
	} else if ((mp->pixelformat == V4L2_PIX_FMT_HSV24) || (mp->pixelformat == V4L2_PIX_FMT_HSVM)) {
		mp->colorspace = V4L2_COLORSPACE_SRGB;
#if 0
	} else if (mp->width <= 720) {
		mp->colorspace = V4L2_COLORSPACE_SMPTE170M;
	} else {
		mp->colorspace = V4L2_COLORSPACE_REC709;
	}
#else
	} else {
		mp->colorspace = V4L2_COLORSPACE_SMPTE170M;
	}
#endif
	memset(mp->reserved, 0, sizeof(mp->reserved));

	return 0;
}

void cvi_vip_buf_queue(struct cvi_base_vdev *vdev, struct cvi_vip_buffer *b)
{
	unsigned long flags;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	list_add_tail(&b->list, &vdev->rdy_queue);
	++vdev->num_rdy;
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);
}

struct cvi_vip_buffer *cvi_vip_next_buf(struct cvi_base_vdev *vdev)
{
	unsigned long flags;
	struct cvi_vip_buffer *b = NULL;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	if (!list_empty(&vdev->rdy_queue))
		b = list_first_entry(&vdev->rdy_queue,
			struct cvi_vip_buffer, list);
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);

	return b;
}

struct cvi_vip_buffer *cvi_vip_buf_remove(struct cvi_base_vdev *vdev)
{
	unsigned long flags;
	struct cvi_vip_buffer *b = NULL;

	if (vdev->num_rdy == 0)
		return b;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	if (!list_empty(&vdev->rdy_queue)) {
		b = list_first_entry(&vdev->rdy_queue,
			struct cvi_vip_buffer, list);
		list_del_init(&b->list);
		--vdev->num_rdy;
	}
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);

	return b;
}

void cvi_vip_buf_cancel(struct cvi_base_vdev *vdev)
{
	unsigned long flags;
	struct cvi_vip_buffer *b = NULL;

	if (vdev->num_rdy == 0)
		return;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	while (!list_empty(&vdev->rdy_queue)) {
		b = list_first_entry(&vdev->rdy_queue,
			struct cvi_vip_buffer, list);
		list_del_init(&b->list);
		--vdev->num_rdy;
		vb2_buffer_done(&b->vb.vb2_buf, VB2_BUF_STATE_ERROR);
	}
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);
}

static void img_left_tile_cfg(struct cvi_img_vdev *idev, bool sc_need_check[])
{
	u8 i;
	struct sclr_img_cfg *cfg = sclr_img_get_cfg(idev->img_type);
	struct sclr_mem mem = cfg->mem;
	struct cvi_vip_dev *bdev = NULL;

	bdev = container_of(idev, struct cvi_vip_dev, img_vdev[idev->dev_idx]);
	dprintk(VIP_DBG, "img-%d: tile on left.\n", idev->dev_idx);

#ifdef TILE_ON_IMG
	mem.width = (mem.width >> 1) + TILE_GUARD_PIXEL;
	sclr_img_set_mem(idev->img_type, &mem, false);
	dprintk(VIP_DBG, "img start_x(%d) width(%d).\n", mem.start_x, mem.width);
#endif
	for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
		if (!sc_need_check[i])
			continue;
		bdev->sc_vdev[i].tile_mode = sclr_tile_cal_size(i);
		if (!sclr_left_tile(i, mem.width))
			atomic_set(&bdev->sc_vdev[i].job_state, CVI_VIP_IDLE);
	}
}

static void img_right_tile_cfg(struct cvi_img_vdev *idev, bool sc_need_check[])
{
	u8 i;
	struct sclr_img_cfg *cfg = sclr_img_get_cfg(idev->img_type);
	struct sclr_mem mem = cfg->mem;
	struct cvi_vip_dev *bdev = NULL;
	u32 sc_offset = (mem.width >> 1) - TILE_GUARD_PIXEL;

	bdev = container_of(idev, struct cvi_vip_dev, img_vdev[idev->dev_idx]);
	dprintk(VIP_DBG, "img-%d: tile on right.\n", idev->dev_idx);

#ifdef TILE_ON_IMG
	mem.start_x += sc_offset;
	mem.width = mem.width - sc_offset;
	sclr_img_set_mem(idev->img_type, &mem, false);
	dprintk(VIP_DBG, "img start_x(%d) width(%d).\n", mem.start_x, mem.width);
#endif
	for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
		if (!sc_need_check[i])
			continue;
		if (!sclr_right_tile(i, sc_offset))
			atomic_set(&bdev->sc_vdev[i].job_state, CVI_VIP_IDLE);
	}
}

static void cvi_img_device_run(struct cvi_img_vdev *idev, bool sc_need_check[])
{
	struct vb2_buffer *vb2_buf;
	struct cvi_vip_dev *bdev = NULL;
	u64 addr[3];
	u8 i;
	struct sclr_top_cfg *top_cfg = sclr_top_get_cfg();

	bdev = container_of(idev, struct cvi_vip_dev, img_vdev[idev->dev_idx]);

	// only update hw if not-tile or at left-tile
	if (!idev->is_tile || !idev->is_work_on_r_tile) {
		if (!(debug & BIT(2)) && idev->clk)
			clk_enable(idev->clk);

		if (idev->is_online_from_isp)
			cvi_img_update(idev, &idev->ol_vpss_grp_cfg[idev->job_grp]);
		else
			cvi_img_update(idev, &idev->vpss_grp_cfg);
		idev->is_cmdq = false;
		for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
			if (sc_need_check[i]) {
				cvi_sc_device_run(&bdev->sc_vdev[i], idev->is_tile, idev->is_work_on_r_tile,
					idev->job_grp);
				top_cfg->sclr_enable[i] = true;
				bdev->sc_vdev[i].is_cmdq = false;
			}
		}
	}

	if (!idev->is_online_from_isp) {
		struct cvi_vip_buffer *b = cvi_vip_next_buf((struct cvi_base_vdev *)idev);

		if (!b) {
			dprintk(VIP_ERR, "img%d no buffer\n", idev->dev_idx);
			return;
		}

		vb2_buf = &b->vb.vb2_buf;

		dprintk(VIP_DBG, "update img-buf: 0x%lx-0x%lx-0x%lx\n",
		    vb2_buf->planes[0].m.userptr, vb2_buf->planes[1].m.userptr,
		    vb2_buf->planes[2].m.userptr);

		for (i = 0; i < 3; i++) {
			addr[i] = vb2_buf->planes[i].m.userptr;
			if (!(vb2_buf->planes[i].m.userptr & 0x100000000))
				addr[i] += 0x100000000;
		}

		sclr_img_set_addr(idev->img_type, addr[0], addr[1], addr[2]);
	}

	//spin_lock_irqsave(&dev->job_lock, flags);
	idev->job_flags |= TRANS_RUNNING;
	//spin_unlock_irqrestore(&dev->job_lock, flags);
	if (idev->is_tile) {
		if (!idev->is_work_on_r_tile) {
			idev->tile_mode = 0;
			for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
				if (!sc_need_check[i])
					continue;
				bdev->sc_vdev[i].tile_mode = sclr_tile_cal_size(i);
				idev->tile_mode |= bdev->sc_vdev[i].tile_mode;
			}
			if (!(idev->tile_mode & SCL_TILE_LEFT)) {
				dprintk(VIP_INFO, "Only right tile.\n");
				idev->is_work_on_r_tile = true;
			}
			if (!(idev->tile_mode & SCL_TILE_RIGHT))
				dprintk(VIP_INFO, "Only left tile.\n");
		}
		if (idev->tile_mode & SCL_TILE_LEFT)
			img_left_tile_cfg(idev, sc_need_check);
		else if (idev->tile_mode & SCL_TILE_RIGHT)
			img_right_tile_cfg(idev, sc_need_check);

		if (debug & BIT(1)) {
			idev->tile_mode = SCL_TILE_LEFT;
			for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
				if (!sc_need_check[i])
					continue;
				bdev->sc_vdev[i].tile_mode = SCL_TILE_LEFT;
			}
		}
	}

	sclr_top_set_cfg(top_cfg);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	ktime_get_ts64(&idev->ts_start);
#else
	ktime_get_ts(&idev->ts_start);
#endif
	if (!bdev->disp_online)
		sclr_img_start(idev->img_type);
}

u8 _gop_get_bpp(enum sclr_gop_format fmt)
{
	return (fmt == SCL_GOP_FMT_ARGB8888) ? 4 :
		(fmt == SCL_GOP_FMT_256LUT) ? 1 : 2;
}

int cvi_vip_set_rgn_cfg(const u8 inst, const struct cvi_rgn_cfg *cfg, const struct sclr_size *size)
{
	struct sclr_gop_cfg *gop_cfg = sclr_gop_get_cfg(inst);
	struct sclr_gop_ow_cfg *ow_cfg;
	u8 i;

	gop_cfg->gop_ctrl.raw &= ~0xfff;
	gop_cfg->gop_ctrl.b.hscl_en = cfg->hscale_x2;
	gop_cfg->gop_ctrl.b.vscl_en = cfg->vscale_x2;
	gop_cfg->gop_ctrl.b.colorkey_en = cfg->colorkey_en;
	gop_cfg->colorkey = cfg->colorkey;

	for (i = 0; i < cfg->num_of_rgn; ++i) {
		u8 bpp = _gop_get_bpp(cfg->param[i].fmt);

		ow_cfg = &gop_cfg->ow_cfg[i];
		gop_cfg->gop_ctrl.raw |= BIT(i);

		ow_cfg->fmt = cfg->param[i].fmt;
		ow_cfg->addr = cfg->param[i].phy_addr;
		ow_cfg->pitch = cfg->param[i].stride;
		if (cfg->param[i].rect.left < 0) {
			ow_cfg->start.x = 0;
			ow_cfg->addr -= bpp * cfg->param[i].rect.left;
			ow_cfg->img_size.w = cfg->param[i].rect.width + cfg->param[i].rect.left;
		} else if ((cfg->param[i].rect.left + cfg->param[i].rect.width) > size->w) {
			ow_cfg->start.x = cfg->param[i].rect.left;
			ow_cfg->img_size.w = size->w - cfg->param[i].rect.left;
			ow_cfg->mem_size.w = cfg->param[i].stride;
		} else {
			ow_cfg->start.x = cfg->param[i].rect.left;
			ow_cfg->img_size.w = cfg->param[i].rect.width;
			ow_cfg->mem_size.w = cfg->param[i].stride;
		}

		if (cfg->param[i].rect.top < 0) {
			ow_cfg->start.y = 0;
			ow_cfg->addr -= ow_cfg->pitch * cfg->param[i].rect.top;
			ow_cfg->img_size.h = cfg->param[i].rect.height + cfg->param[i].rect.top;
		} else if ((cfg->param[i].rect.top + cfg->param[i].rect.height) > size->h) {
			ow_cfg->start.y = cfg->param[i].rect.top;
			ow_cfg->img_size.h = size->h - cfg->param[i].rect.top;
		} else {
			ow_cfg->start.y = cfg->param[i].rect.top;
			ow_cfg->img_size.h = cfg->param[i].rect.height;
		}

		ow_cfg->end.x = ow_cfg->start.x +
				(ow_cfg->img_size.w << gop_cfg->gop_ctrl.b.hscl_en) - gop_cfg->gop_ctrl.b.hscl_en;
		ow_cfg->end.y = ow_cfg->start.y +
				(ow_cfg->img_size.h << gop_cfg->gop_ctrl.b.vscl_en) - gop_cfg->gop_ctrl.b.vscl_en;
		ow_cfg->mem_size.w = ALIGN(ow_cfg->img_size.w * bpp, GOP_ALIGNMENT);
		ow_cfg->mem_size.h = ow_cfg->img_size.h;

		dprintk(VIP_INFO, "gop(%d) fmt(%d) rect(%d %d %d %d) addr(%llx) pitch(%d).\n", inst
			, ow_cfg->fmt, ow_cfg->start.x, ow_cfg->start.y, ow_cfg->img_size.w, ow_cfg->img_size.h
			, ow_cfg->addr, ow_cfg->pitch);
		sclr_gop_ow_set_cfg(inst, i, ow_cfg, true);
	}

	sclr_gop_set_cfg(inst, gop_cfg, true);

	return 0;
}

/**
 * cvi_vip_try_schedule - check if img is ready for a job
 *
 * @param idev: img-dev who to run.
 * @param grp_id: vpss grp settings to run.
 *                0 for offline or online-snr0; 1 for online-srn1.
 * @param is_online: trig online or offline job.
 * @return: 0 if ready
 */
int cvi_vip_try_schedule(struct cvi_img_vdev *idev, u8 grp_id, bool is_online)
{
	struct cvi_vip_dev *bdev = NULL;
	unsigned long flags_out[4], flags_img, flags_job;
	u8 i;
	bool check_img_buffer = (idev->input_type == CVI_VIP_INPUT_MEM);
	bool sc_need_check[CVI_VIP_SC_MAX] = { [0 ... CVI_VIP_SC_MAX - 1] = false };
	bool sc_locked[CVI_VIP_SC_MAX] = { [0 ... CVI_VIP_SC_MAX - 1] = false };
	bool sc_4k = false, sc_2688 = false;
	bool tile = false;
	bool sc_enable = false;
	struct cvi_vpss_grp_cfg *grp_cfg;
	struct cvi_vpss_chn_cfg *chn_cfg;

	bdev = container_of(idev, struct cvi_vip_dev, img_vdev[idev->dev_idx]);

	spin_lock_irqsave(&idev->job_lock, flags_job);
	if (is_online ^ idev->is_online_from_isp) {
		dprintk(VIP_WARN, "Caller is %pS, img-%d try trig %s job but current mode is %s\n",
			__builtin_return_address(0), idev->dev_idx,
			(is_online) ? "online" : "offline",
			(idev->is_online_from_isp) ? "online" : "offline");
		goto job_unlock;
	}
	if (cvi_vip_job_is_queued(idev)) {
		dprintk(VIP_WARN, "Caller is %pS, On job queue already\n",
			__builtin_return_address(0));
		goto job_unlock;
	}

	spin_lock_irqsave(&idev->rdy_lock, flags_img);
	// check if instances is on
	if (!idev->vb_q.streaming) {
		dprintk(VIP_WARN, "Caller is %pS, img-%d needs to be on.\n",
			__builtin_return_address(0), idev->dev_idx);
		goto img_unlock;
	}
	// if img_in online, then buffer no needed
	if (check_img_buffer) {
		if (list_empty(&idev->rdy_queue)) {
			dprintk(VIP_WARN, "Caller is %pS, No input buffers available.\n",
				__builtin_return_address(0));
			goto img_unlock;
		}
	}

	idev->job_grp = grp_id;
	cvi_img_get_sc_bound(idev, sc_need_check);

	// check sc's queue if bounding
	for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
		if (!sc_need_check[i])
			continue;

		sc_enable = true;
		spin_lock_irqsave(&bdev->sc_vdev[i].rdy_lock, flags_out[i]);
		sc_locked[i] = true;
		if (!atomic_read(&bdev->sc_vdev[i].is_streaming)) {
			dprintk(VIP_WARN, "sc-%d needs to be on.\n", i);
			goto sc_unlock;
		}
		if (atomic_read(&bdev->sc_vdev[i].job_state) == CVI_VIP_RUNNING) {
			dprintk(VIP_WARN, "sc-%d busy.\n", i);
			goto sc_unlock;
		}
		if (idev->is_online_from_isp) {
			if (bdev->sc_vdev[i].ol_num_rdy[grp_id] == 0 && !idev->is_tile) {
				dprintk(VIP_WARN, "No sc-%d buffer available.\n", i);
				goto sc_unlock;
			}
		} else {
			if (bdev->sc_vdev[i].num_rdy == 0 && !idev->is_tile) {
				dprintk(VIP_WARN, "No sc-%d buffer available.\n", i);
				goto sc_unlock;
			}
		}
		if (idev->is_online_from_isp)
			chn_cfg = &bdev->sc_vdev[i].ol_vpss_chn_cfg[grp_id];
		else
			chn_cfg = &bdev->sc_vdev[i].vpss_chn_cfg;
		if (chn_cfg->dst_rect.width > SCL_MAX_WIDTH) {
			sc_2688 = true;
			if (chn_cfg->dst_rect.width > BIT(12))
				sc_4k = true;
		}
	}
	if (!sc_enable)
		goto img_unlock;

	if (idev->is_online_from_isp)
		grp_cfg = &idev->ol_vpss_grp_cfg[grp_id];
	else
		grp_cfg = &idev->vpss_grp_cfg;

	// sc's size at most 4096, line buffer at most 2688
	tile = sc_4k ||
	       (grp_cfg->crop.width > BIT(12)) ||
	       (sc_2688 && (grp_cfg->crop.width > SCL_MAX_WIDTH));
	if (debug & BIT(0))
		tile = true;
	if (tile && sc_need_check[CVI_VIP_SC_D] && bdev->disp_online) {
		dprintk(VIP_WARN, "tile can't work if disp online.\n");
		goto sc_unlock;
	}

	// job status update
	idev->is_tile = tile;
	if (idev->is_tile)
		idev->is_work_on_r_tile ^= true;
	for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
		if (!sc_locked[i])
			continue;

		if (atomic_cmpxchg(&bdev->sc_vdev[i].job_state, CVI_VIP_IDLE, CVI_VIP_RUNNING)) {
			dprintk(VIP_ERR, " sc(%d) still busy. Reject\n", i);
			goto sc_unlock;
		}

		bdev->sc_vdev[i].job_grp = grp_id;
		spin_unlock_irqrestore(&bdev->sc_vdev[i].rdy_lock,
				       flags_out[i]);
	}

	idev->job_flags |= TRANS_QUEUED;
	spin_unlock_irqrestore(&idev->rdy_lock, flags_img);

	// hw operations
	cvi_img_device_run(idev, sc_need_check);

	spin_unlock_irqrestore(&idev->job_lock, flags_job);
	return 0;

sc_unlock:
	for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
		if (!sc_locked[i])
			continue;
		spin_unlock_irqrestore(&bdev->sc_vdev[i].rdy_lock,
				       flags_out[i]);
	}
img_unlock:
	spin_unlock_irqrestore(&idev->rdy_lock, flags_img);
job_unlock:
	spin_unlock_irqrestore(&idev->job_lock, flags_job);
	return -1;
}

/**
 * cvi_vip_job_finish - bottom-half of img/sc-isr to check
 *                      if current job is done and ready for next one.
 *
 * @param idev: img-dev who is running.
 */
void cvi_vip_job_finish(struct cvi_img_vdev *idev)
{
	struct cvi_vip_dev *bdev = container_of(idev, struct cvi_vip_dev, img_vdev[idev->dev_idx]);
	bool sc_need_check[CVI_VIP_SC_MAX] = { [0 ... CVI_VIP_SC_MAX - 1] = false };
	u8 i;

	// check if all dev idle
	if (idev->job_flags & TRANS_RUNNING)
		return;

	cvi_img_get_sc_bound(idev, sc_need_check);

	for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
		if (!sc_need_check[i])
			continue;

		if (atomic_read(&bdev->sc_vdev[i].job_state) != CVI_VIP_IDLE)
			return;
	}

	dprintk(VIP_INFO, "img(%d) job_grp(%d) %s done\n", idev->dev_idx, idev->job_grp,
		idev->is_tile ? (idev->is_work_on_r_tile ? "right tile" : "left tile") : "");

	if (!idev->is_tile || !idev->tile_mode) {
		struct sclr_top_cfg *cfg = sclr_top_get_cfg();
		struct cvi_sc_vdev *sdev;

		idev->is_tile = false;
		idev->is_work_on_r_tile = true;
		idev->tile_mode = 0;

		// disable ip & clk
		for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
			if (!sc_need_check[i])
				continue;
			sdev = &bdev->sc_vdev[i];
			cfg->sclr_enable[sdev->dev_idx] = false;
		}
		sclr_top_set_cfg(cfg);

		for (i = CVI_VIP_SC_D; i < CVI_VIP_SC_MAX; ++i) {
			if (!sc_need_check[i])
				continue;
			sdev = &bdev->sc_vdev[i];
			if (!(debug & BIT(2)) && sdev->clk && __clk_is_enabled(sdev->clk))
				clk_disable(sdev->clk);
		}
		if (!(debug & BIT(2)) && idev->clk && __clk_is_enabled(idev->clk))
			clk_disable(idev->clk);

		// wakeup user who is poll wait
		dprintk(VIP_INFO, "img(%d) finish. frame_number:%d\n", idev->dev_idx, idev->frame_number);
		cvi_img_v4l2_event_queue(idev, V4L2_EVENT_CVI_VIP_VPSS_EOF, idev->job_grp, 0);
		idev->IntMask = 0;
	}

	idev->job_flags &= ~(TRANS_QUEUED);

	if (!idev->is_online_from_isp) {
		if (idev->tile_mode & SCL_TILE_RIGHT)
			tasklet_hi_schedule(&idev->job_work);
	} else
		cvi_sc_trigger_post(bdev);
}

bool cvi_vip_job_is_queued(struct cvi_img_vdev *idev)
{
	return (idev->job_flags & TRANS_QUEUED);
}

u32 cvi_sc_cfg_cb(struct sc_cfg_cb *post_para)
{
	u32 ret = -1;
	int img_dev;
	u8 grp_id = post_para->snr_num;

	if (!post_para->dev) {
		dprintk(VIP_ERR, "NULL ptr!\n");
		return -1;
	}
	if (grp_id > 1) {
		dprintk(VIP_DBG, "post_para->snr_num error\n");
		return -1;
	}

	for (img_dev = 0; img_dev < CVI_VIP_IMG_MAX; ++img_dev) {
		if (post_para->dev->img_vdev[img_dev].ol_vpss_grp_cfg[grp_id].is_online) {
			dprintk(VIP_INFO, "grp_id(%d) is online on img_dev(%d)\n", grp_id, img_dev);
			break;
		}
	}
	if (img_dev == CVI_VIP_IMG_MAX) {
		dprintk(VIP_DBG, "did not find online img_dev for this grp_id(%d)\n", grp_id);
		return -1;
	}

	post_para->dev->img_vdev[img_dev].isp_trig_cnt[grp_id]++;
	if (post_para->dev->img_vdev[img_dev].is_online_from_isp) {
		dprintk(VIP_DBG, "online trig for snr-%d. img:%d\n", grp_id, img_dev);
		ret = cvi_vip_try_schedule(&post_para->dev->img_vdev[img_dev], grp_id, true);
	} else
		dprintk(VIP_DBG, "Currently img(%d) is in offline mode\n", img_dev);
	if (ret)
		post_para->dev->img_vdev[img_dev].isp_trig_fail_cnt[grp_id]++;
	return ret;
}
EXPORT_SYMBOL_GPL(cvi_sc_cfg_cb);

void cvi_sc_trigger_post(struct cvi_vip_dev *dev)
{
	if (!dev)
		return;

	isp_ol_sc_trig_post(&dev->isp_vdev);
}
EXPORT_SYMBOL_GPL(cvi_sc_trigger_post);

static irqreturn_t scl_isr(int irq, void *_dev)
{
	union sclr_intr intr_status = sclr_intr_status();
	u8 cmdq_intr_status = sclr_cmdq_intr_status();

	if (cmdq_intr_status) {
		sclr_cmdq_intr_clr(cmdq_intr_status);
		dprintk(VIP_INFO, "cmdq_intr_status(0x%x)\n", cmdq_intr_status);
	}

	sclr_intr_clr(intr_status);

	dprintk(VIP_ISP_IRQ, "status(0x%x)\n", intr_status.raw);
	disp_irq_handler(intr_status, _dev);
	img_irq_handler(intr_status, cmdq_intr_status, _dev);
	sc_irq_handler(intr_status, _dev);

	return IRQ_HANDLED;
}

static irqreturn_t dwa_isr(int irq, void *_dev)
{
	u8 intr_status = dwa_intr_status();

	dwa_intr_clr(intr_status);
	dwa_irq_handler(intr_status, _dev);
	return IRQ_HANDLED;
}

static irqreturn_t isp_isr(int irq, void *_dev)
{
	struct cvi_vip_dev *bdev = _dev;
	struct isp_ctx *ctx = &bdev->isp_vdev.ctx;
	union isp_csi_intr cbdg_sts, cbdg_sts_b;
	union isp_intr top_sts = isp_intr_status(ctx);

	cbdg_sts_b.raw = 0;

	cbdg_sts = isp_csi_intr_status(ctx, ISP_PRERAW_A);
	isp_csi_intr_clr(ctx, cbdg_sts, ISP_PRERAW_A);

	if (ctx->is_dual_sensor) {
		cbdg_sts_b = isp_csi_intr_status(ctx, ISP_PRERAW_B);
		isp_csi_intr_clr(ctx, cbdg_sts_b, ISP_PRERAW_B);
	}

	isp_intr_clr(ctx, top_sts);
	isp_irq_handler(top_sts, cbdg_sts, _dev, cbdg_sts_b);
	return IRQ_HANDLED;
}
static int _get_reserved_mem(struct platform_device *pdev,
	uint64_t *addr, uint64_t *size)
{
	struct device_node *target = NULL;
	struct reserved_mem *prmem = NULL;

	if (!pdev) {
		dev_err(&pdev->dev, "[VIP] null pointer\n");
		return -EINVAL;
	}

	target = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
	if (!target) {
		dev_err(&pdev->dev, "[VIP] No %s specified\n", "memory-region");
		return -EINVAL;
	}

	prmem = of_reserved_mem_lookup(target);
	of_node_put(target);

	if (!prmem) {
		dev_err(&pdev->dev, "[VIP]: cannot acquire memory-region\n");
		return -EINVAL;
	}

	dprintk(VIP_INFO, "pool name = %s, size = 0x%llx, base = 0x%llx\n",
		prmem->name, prmem->size, prmem->base);

	*addr = prmem->base;
	*size = prmem->size;

	return 0;
}

static int _init_resources(struct platform_device *pdev)
{
	int rc = 0;
#if (DEVICE_FROM_DTS)
	int irq_num[3];
	static const char * const irq_name[] = {"sc", "dwa", "isp"};
	static const char * const clk_sys_name[] = {"clk_sys_0", "clk_sys_1", "clk_sys_2"};
	static const char * const clk_sc_top_name = "clk_sc_top";
	struct resource *res = NULL;
	void *reg_base[6];
	struct cvi_vip_dev *dev;
	int i;

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get cvi_vip drvdata\n");
		return -EINVAL;
	}

	for (i = 0; i < 5; ++i) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		reg_base[i] = devm_ioremap(&pdev->dev, res->start,
						    res->end - res->start);
#else
		reg_base[i] = devm_ioremap_nocache(&pdev->dev, res->start,
						    res->end - res->start);
#endif
		dprintk(VIP_INFO, "(%d) res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%p).\n",
			i, res->start, res->end, reg_base[i]);
	}
	sclr_set_base_addr(reg_base[0]);
	dwa_set_base_addr(reg_base[1]);
	vip_set_base_addr(reg_base[2]);
	isp_set_base_addr(reg_base[3]);
	dphy_set_base_addr(reg_base[4]);

	isp_mempool_setup();

	/* Interrupt */
	for (i = 0; i < ARRAY_SIZE(irq_name); ++i) {
		irq_num[i] = platform_get_irq_byname(pdev, irq_name[i]);
		if (irq_num[i] < 0) {
			dev_err(&pdev->dev, "No IRQ resource for %s\n",
				irq_name[i]);
			return -ENODEV;
		}
		dprintk(VIP_INFO, "irq(%d) for %s get from platform driver.\n",
			irq_num[i], irq_name[i]);
	}
	dev->irq_num_scl = irq_num[0];
	dev->irq_num_dwa = irq_num[1];
	dev->irq_num_isp = irq_num[2];

	for (i = 0; i < ARRAY_SIZE(clk_sys_name); ++i) {
		dev->clk_sys[i] = devm_clk_get(&pdev->dev, clk_sys_name[i]);
		if (IS_ERR(dev->clk_sys[i])) {
			pr_err("Cannot get clk for clk_sys_%d\n", i);
			dev->clk_sys[i] = NULL;
		}
	}
	dev->clk_sc_top = devm_clk_get(&pdev->dev, clk_sc_top_name);
	if (IS_ERR(dev->clk_sc_top)) {
		pr_err("Cannot get clk for clk_sc_top\n");
		dev->clk_sc_top = NULL;
	}

	if (device_property_read_u32(&pdev->dev, "clock-freq-vip-sys1", &dev->clk_sys1_freq) != 0)
		dev->clk_sys1_freq = 300000000UL;
#endif

	return rc;
}

static int cvi_vip_probe(struct platform_device *pdev)
{
	int rc = 0;
	u8 i = 0;
	struct cvi_vip_dev *dev;

	/* allocate main structure */
	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	/* initialize locks */
	spin_lock_init(&dev->lock);
	mutex_init(&dev->mutex);

	dev_set_drvdata(&pdev->dev, dev);

	/* register v4l2_device */
	snprintf(dev->v4l2_dev.name, sizeof(dev->v4l2_dev.name), "cvi_vip");
	rc = v4l2_device_register(&pdev->dev, &dev->v4l2_dev);
	if (rc) {
		dev_err(&pdev->dev, "Failed to register v4l2 device\n");
		goto err_v4l2_reg;
	}

	// get hw-resources
	rc = _init_resources(pdev);
	if (rc)
		goto err_irq;

	// for dwa - m2m
	rc = dwa_create_instance(pdev);
	if (rc) {
		dprintk(VIP_ERR, "Failed to create dwa instance\n");
		goto err_dwa_reg;
	}

	// for img(2) - cap
	rc = img_create_instance(pdev);
	if (rc) {
		dprintk(VIP_ERR, "Failed to create img instance\n");
		goto err_img_reg;
	}

	// for sc(4) - out
	rc = sc_create_instance(pdev);
	if (rc) {
		dprintk(VIP_ERR, "Failed to create sc instance\n");
		goto err_sc_reg;
	}

	// for disp - out
	rc = disp_create_instance(pdev);
	if (rc) {
		dprintk(VIP_ERR, "Failed to create disp instance\n");
		goto err_disp_reg;
	}

	// for isp - cap
	rc = isp_create_instance(pdev);
	if (rc) {
		dprintk(VIP_ERR, "Failed to create isp instance\n");
		goto err_isp_reg;
	}

	if (vip_clk_freq)
		dev->clk_sys1_freq = vip_clk_freq;
	if (dev->clk_sys[1])
		clk_set_rate(clk_get_parent(dev->clk_sys[1]), dev->clk_sys1_freq);

	sclr_ctrl_init(false);
	if (smooth) {
		sclr_disp_cfg_setup_from_reg();
	} else {
		if (!(debug & BIT(2)) && dev->clk_sc_top && __clk_is_enabled(dev->clk_sc_top))
			clk_disable_unprepare(dev->clk_sc_top);
	}

	VIP_CLK_RATIO_CONFIG(ISP_TOP, 0x10);
	VIP_CLK_RATIO_CONFIG(DWA, 0x10);
	VIP_CLK_RATIO_CONFIG(IMG_D, 0x10);
	VIP_CLK_RATIO_CONFIG(IMG_V, 0x10);
	VIP_CLK_RATIO_CONFIG(SC_D, 0x10);
	VIP_CLK_RATIO_CONFIG(SC_V1, 0x10);
	VIP_CLK_RATIO_CONFIG(SC_V2, 0x10);
	VIP_CLK_RATIO_CONFIG(SC_V3, 0x10);

	vip_sys_set_offline(VIP_SYS_AXI_BUS_SC_TOP, true);
	vip_sys_set_offline(VIP_SYS_AXI_BUS_ISP_RAW, true);
	vip_sys_set_offline(VIP_SYS_AXI_BUS_ISP_YUV, true);

	//dwa_init();
	//dwa_intr_ctrl(0x01);

	dev->disp_online = false;

	dprintk(VIP_DBG, "hw init done\n");

	if (devm_request_irq(&pdev->dev, dev->irq_num_scl, scl_isr, IRQF_SHARED,
		 "CVI_VIP_SCL", dev)) {
		dev_err(&pdev->dev, "Unable to request scl IRQ(%d)\n",
				dev->irq_num_scl);
		return -EINVAL;
	}

	if (devm_request_irq(&pdev->dev, dev->irq_num_dwa, dwa_isr, IRQF_SHARED,
		 "CVI_VIP_DWA", dev)) {
		dev_err(&pdev->dev, "Unable to request dwa IRQ(%d)\n",
				dev->irq_num_dwa);
		return -EINVAL;
	}

	if (devm_request_irq(&pdev->dev, dev->irq_num_isp, isp_isr, IRQF_SHARED,
		 "CVI_VIP_ISP", dev)) {
		dev_err(&pdev->dev, "Unable to request isp IRQ(%d)\n",
				dev->irq_num_isp);
		return -EINVAL;
	}

	dprintk(VIP_DBG, "done with rc(%d).\n", rc);
	return rc;

err_isp_reg:
	video_unregister_device(&dev->disp_vdev.vdev);
err_disp_reg:
	for (i = 0; i < CVI_VIP_SC_MAX; ++i)
		video_unregister_device(&dev->sc_vdev[i].vdev);
err_sc_reg:
	for (i = 0; i < CVI_VIP_IMG_MAX; ++i)
		video_unregister_device(&dev->img_vdev[i].vdev);
err_img_reg:
	video_unregister_device(&dev->dwa_vdev.vdev);
err_dwa_reg:
err_irq:
	v4l2_device_unregister(&dev->v4l2_dev);
err_v4l2_reg:
	dev_set_drvdata(&pdev->dev, NULL);

	dprintk(VIP_DBG, "failed with rc(%d).\n", rc);
	return rc;
}

/*
 * bmd_remove - device remove method.
 * @pdev: Pointer of platform device.
 */
static int cvi_vip_remove(struct platform_device *pdev)
{
	u8 i = 0;
	struct cvi_vip_dev *dev;

	isp_destroy_instance(pdev);
	dwa_destroy_instance(pdev);
	img_destroy_instance(pdev);
	sc_destroy_instance(pdev);
	disp_destroy_instance(pdev);

	if (!pdev) {
		dev_err(&pdev->dev, "invalid param");
		return -EINVAL;
	}

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get cvi_vip drvdata");
		return 0;
	}
	video_unregister_device(&dev->isp_vdev.vdev);
	video_unregister_device(&dev->disp_vdev.vdev);
	for (i = 0; i < CVI_VIP_SC_MAX; ++i)
		video_unregister_device(&dev->sc_vdev[i].vdev);
	for (i = 0; i < CVI_VIP_IMG_MAX; ++i)
		video_unregister_device(&dev->img_vdev[i].vdev);
	video_unregister_device(&dev->dwa_vdev.vdev);
	v4l2_device_unregister(&dev->v4l2_dev);
	dev_set_drvdata(&pdev->dev, NULL);

	return 0;
}

static int vip_suspend(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);
	return 0;
}

static int vip_resume(struct device *dev)
{
	dev_info(dev, "%s\n", __func__);

	sclr_ctrl_init(true);
	VIP_CLK_RATIO_CONFIG(ISP_TOP, 0x10);
	VIP_CLK_RATIO_CONFIG(DWA, 0x10);
	VIP_CLK_RATIO_CONFIG(IMG_D, 0x10);
	VIP_CLK_RATIO_CONFIG(IMG_V, 0x10);
	VIP_CLK_RATIO_CONFIG(SC_D, 0x10);
	VIP_CLK_RATIO_CONFIG(SC_V1, 0x10);
	VIP_CLK_RATIO_CONFIG(SC_V2, 0x10);
	VIP_CLK_RATIO_CONFIG(SC_V3, 0x10);

	vip_sys_set_offline(VIP_SYS_AXI_BUS_SC_TOP, true);
	vip_sys_set_offline(VIP_SYS_AXI_BUS_ISP_RAW, true);
	vip_sys_set_offline(VIP_SYS_AXI_BUS_ISP_YUV, true);

	return 0;
}

static const struct of_device_id cvi_vip_dt_match[] = {
	{.compatible = "cvitek,vip"},
	{}
};

static SIMPLE_DEV_PM_OPS(vip_pm_ops, vip_suspend,
				vip_resume);

#if (!DEVICE_FROM_DTS)
static void cvi_vip_pdev_release(struct device *dev)
{
}

static struct platform_device cvi_vip_pdev = {
	.name		= "vip",
	.dev.release	= cvi_vip_pdev_release,
};
#endif

static struct platform_driver cvi_vip_pdrv = {
	.probe      = cvi_vip_probe,
	.remove     = cvi_vip_remove,
	.driver     = {
		.name		= "vip",
		.owner		= THIS_MODULE,
#if (DEVICE_FROM_DTS)
		.of_match_table	= cvi_vip_dt_match,
#endif
		.pm		= &vip_pm_ops,
	},
};

static int __init cvi_vip_init(void)
{
	int rc;

	dprintk(VIP_INFO, " +\n");
	#if (DEVICE_FROM_DTS)
	rc = platform_driver_register(&cvi_vip_pdrv);
	#else
	rc = platform_device_register(&cvi_vip_pdev);
	if (rc)
		return rc;

	rc = platform_driver_register(&cvi_vip_pdrv);
	if (rc)
		platform_device_unregister(&cvi_vip_pdev);
	#endif

	return rc;
}

static void __exit cvi_vip_exit(void)
{
	dprintk(VIP_INFO, " +\n");
	platform_driver_unregister(&cvi_vip_pdrv);
	#if (!DEVICE_FROM_DTS)
	platform_device_unregister(&cvi_vip_pdev);
	#endif
}

MODULE_DESCRIPTION("Cvitek Video Driver");
MODULE_AUTHOR("Jammy Huang");
MODULE_LICENSE("GPL");
module_init(cvi_vip_init);
module_exit(cvi_vip_exit);