/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_vip_dwa.c
 * Description: dewarp driver
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/videodev2.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-event.h>
#include <media/v4l2-mem2mem.h>

#include "vip/vip_common.h"
#include "vip/dwa.h"

#include "cvi_debug.h"
#include "cvi_vip_core.h"
#include "cvi_vip_dwa.h"
#include "cvi_vip_gdc_proc.h"

#define fh_to_ctx(__fh) container_of(__fh, struct cvi_dwa_ctx, fh)
#define GDC_SHARE_MEM_SIZE (0x8000)

#define HW_TIMEOUT_MS	100

static const char *const CLK_DWA_NAME = "clk_dwa";
static enum hrtimer_restart dwa_timer_handler(struct hrtimer *timer)
{
	struct cvi_dwa_vdev *wdev = container_of(timer, struct cvi_dwa_vdev, timer);
	struct cvi_dwa_ctx *ctx;
	struct vb2_v4l2_buffer *src_buf, *dst_buf;

	dprintk(VIP_WARN, "dwa timeout\n");

	ctx = v4l2_m2m_get_curr_priv(wdev->m2m_dev);
	if (!ctx || !ctx->fh.m2m_ctx) {
		pr_err("%s: null ctx or m2m_ctx\n", __func__);
		return HRTIMER_NORESTART;
	}

	src_buf = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
	dst_buf = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);

	dst_buf->vb2_buf.timestamp = src_buf->vb2_buf.timestamp;
	dst_buf->timecode = src_buf->timecode;
	dst_buf->flags &= ~V4L2_BUF_FLAG_TSTAMP_SRC_MASK;
	dst_buf->flags |= src_buf->flags & V4L2_BUF_FLAG_TSTAMP_SRC_MASK;

	dwa_disable();

	v4l2_m2m_buf_done(src_buf, VB2_BUF_STATE_DONE);
	v4l2_m2m_buf_done(dst_buf, VB2_BUF_STATE_DONE);
	v4l2_m2m_job_finish(wdev->m2m_dev, ctx->fh.m2m_ctx);

	return HRTIMER_NORESTART;
}

static struct cvi_dwa_data *_dwa_get_data(struct cvi_dwa_ctx *ctx,
		enum v4l2_buf_type type)
{
	if (V4L2_TYPE_IS_OUTPUT(type))
		return &ctx->out_data;
	return &ctx->cap_data;
}

static void cvi_dwa_device_run(void *priv)
{
	struct cvi_dwa_ctx *ctx = priv;
	struct vb2_v4l2_buffer *src_buf, *dst_buf;
	struct cvi_dwa_data *src_fmt, *dst_fmt;
	struct dwa_cfg cfg;
	u8 num_of_plane;
	u8 i = 0;
	struct cvi_dwa_vdev *wdev = ctx->wdev;

	dprintk(VIP_DBG, " for m2m_ctx(%p)\n", ctx->fh.m2m_ctx);

	src_buf = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);
	dst_buf = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);

	src_fmt = _dwa_get_data(ctx, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
	dst_fmt = _dwa_get_data(ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);

	memset(&cfg, 0, sizeof(cfg));
	switch (ctx->fmt->fmt) {
	case SCL_FMT_YUV420:
		cfg.pix_fmt = YUV420p;
		num_of_plane = 3;
	break;
	case SCL_FMT_Y_ONLY:
		cfg.pix_fmt = YUV400;
		num_of_plane = 1;
	break;
	case SCL_FMT_RGB_PLANAR:
	default:
		cfg.pix_fmt = RGB888p;
		num_of_plane = 3;
	break;
	};

	cfg.mesh_id = ctx->mesh_id_addr;
	cfg.output_target = ctx->output;
	cfg.bgcolor = ctx->bgcolor;
	cfg.src_width  = src_fmt->w;
	cfg.src_height = src_fmt->h;
	cfg.dst_width  = dst_fmt->w;
	cfg.dst_height = dst_fmt->h;

	for (i = 0; i < num_of_plane; ++i) {
		u64 addr = src_buf->vb2_buf.planes[i].m.userptr;

		if (!(src_buf->vb2_buf.planes[i].m.userptr & 0x100000000))
			addr += 0x100000000;

		cfg.src_buf[i].addrl    = addr;
		cfg.src_buf[i].addrh    = addr >> 32;
		cfg.src_buf[i].pitch    = (src_buf->vb2_buf.planes[i].bytesused > src_fmt->bytesperline[i])
					? src_buf->vb2_buf.planes[i].bytesused
					: src_fmt->bytesperline[i];
		cfg.src_buf[i].offset_x = cfg.src_buf[i].offset_y = 0;

		addr = dst_buf->vb2_buf.planes[i].m.userptr;

		if (!(dst_buf->vb2_buf.planes[i].m.userptr & 0x100000000))
			addr += 0x100000000;

		cfg.dst_buf[i].addrl    = addr;
		cfg.dst_buf[i].addrh    = addr >> 32;
		cfg.dst_buf[i].pitch    = dst_fmt->bytesperline[i];
		cfg.dst_buf[i].offset_x = cfg.src_buf[i].offset_y = 0;
	}

	dprintk(VIP_DBG, "update size src(%d %d) dst(%d %d)\n",
		cfg.src_width, cfg.src_height, cfg.dst_width, cfg.dst_height);
	dprintk(VIP_DBG, "update src-buf: 0x%lx-0x%lx-0x%lx\n",
		src_buf->vb2_buf.planes[0].m.userptr,
		src_buf->vb2_buf.planes[1].m.userptr,
		src_buf->vb2_buf.planes[2].m.userptr);
	dprintk(VIP_DBG, "update dst-buf: 0x%lx-0x%lx-0x%lx\n",
		dst_buf->vb2_buf.planes[0].m.userptr,
		dst_buf->vb2_buf.planes[1].m.userptr,
		dst_buf->vb2_buf.planes[2].m.userptr);
	dprintk(VIP_DBG, "update mesh_id_addr(%#llx)\n", ctx->mesh_id_addr);

	dwa_reset();
	dwa_init();

	hrtimer_init(&wdev->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	wdev->timer.function = dwa_timer_handler;
	hrtimer_start(&wdev->timer, ms_to_ktime(HW_TIMEOUT_MS), HRTIMER_MODE_REL);

	dwa_engine(&cfg);
}

#if 0
static int cvi_dwa_job_ready(void *priv)
{
	// TODO: needed??
	return dwa_is_busy() ? 0 : 1;
}
#endif

static void cvi_dwa_job_abort(void *priv)
{
	// TODO: needed? m2m required
}

static const struct v4l2_m2m_ops cvi_dwa_m2m_ops = {
	.device_run = cvi_dwa_device_run,
	//.job_ready  = cvi_dwa_job_ready,
	.job_abort  = cvi_dwa_job_abort,
};

/*************************************************************************
 *	VB2_OPS definition
 *************************************************************************/
/**
 * call before VIDIOC_REQBUFS to setup buf-queue.
 * nbuffers: number of buffer requested
 * nplanes:  number of plane each buffer
 * sizes:    size of each plane(bytes)
 */
static int cvi_dwa_queue_setup(struct vb2_queue *vq,
	       unsigned int *nbuffers, unsigned int *nplanes,
	       unsigned int sizes[], struct device *alloc_devs[])
{
	struct cvi_dwa_ctx *ctx = vb2_get_drv_priv(vq);
	struct cvi_dwa_data *data = NULL;
	unsigned int planes, p;

	dprintk(VIP_VB2, "%s+\n", V4L2_TYPE_IS_OUTPUT(vq->type) ? "out" : "cap");

	data = _dwa_get_data(ctx, vq->type);
	if (!data)
		return -EINVAL;

	planes = ctx->fmt->buffers;

	for (p = 0; p < planes; ++p)
		sizes[p] = data->sizeimage[p];

	if (vq->num_buffers + *nbuffers < 2)
		*nbuffers = 2 - vq->num_buffers;

	*nplanes = planes;

	dprintk(VIP_INFO, "num_buffer=%d, num_plane=%d\n", *nbuffers, *nplanes);
	for (p = 0; p < *nplanes; p++)
		dprintk(VIP_INFO, "size[%u]=%u\n", p, sizes[p]);

	return 0;
}

/**
 * for VIDIOC_STREAMON, start fill data.
 */
static void cvi_dwa_buf_queue(struct vb2_buffer *vb)
{
	struct cvi_dwa_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);

	dprintk(VIP_VB2, "%s+\n", V4L2_TYPE_IS_OUTPUT(vb->type) ? "out" : "cap");

	v4l2_m2m_buf_queue(ctx->fh.m2m_ctx, vbuf);
}

static struct vb2_v4l2_buffer *_dwa_buf_remove(struct cvi_dwa_ctx *ctx,
					       enum v4l2_buf_type type)
{
	if (V4L2_TYPE_IS_OUTPUT(type))
		return v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
	else
		return v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);
}

static int cvi_dwa_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct cvi_dwa_ctx *ctx = vb2_get_drv_priv(vq);
	int rc = 0;

	dprintk(VIP_VB2, "%s+\n", V4L2_TYPE_IS_OUTPUT(vq->type) ? "out" : "cap");
	if (!(debug & BIT(2)) && ctx->wdev->clk)
		clk_prepare_enable(ctx->wdev->clk);

	return rc;
}

/* abort streaming and wait for last buffer */
static void cvi_dwa_stop_streaming(struct vb2_queue *vq)
{
	struct cvi_dwa_ctx *ctx = vb2_get_drv_priv(vq);
	struct vb2_v4l2_buffer *vb;

	dprintk(VIP_VB2, "%s+\n", V4L2_TYPE_IS_OUTPUT(vq->type) ? "out" : "cap");

	while ((vb = _dwa_buf_remove(ctx, vq->type)))
		v4l2_m2m_buf_done(vb, VB2_BUF_STATE_ERROR);

	if (!(debug & BIT(2)) && ctx->wdev->clk && __clk_is_enabled(ctx->wdev->clk))
		clk_disable_unprepare(ctx->wdev->clk);
}

const struct vb2_ops cvi_dwa_qops = {
	.queue_setup        = cvi_dwa_queue_setup,
	.buf_queue          = cvi_dwa_buf_queue,
	.start_streaming    = cvi_dwa_start_streaming,
	.stop_streaming     = cvi_dwa_stop_streaming,
	.wait_prepare       = vb2_ops_wait_prepare,
	.wait_finish        = vb2_ops_wait_finish,
};

/*************************************************************************
 *	VB2-MEM-OPS definition
 *************************************************************************/
static void *_get_userptr(struct device *dev, unsigned long vaddr,
	unsigned long size, enum dma_data_direction dma_dir)
{
	return (void *)0xdeadbeef;
}

static void _put_userptr(void *buf_priv)
{
}

static const struct vb2_mem_ops cvi_dwa_vb2_mem_ops = {
	.get_userptr = _get_userptr,
	.put_userptr = _put_userptr,
};

/*************************************************************************
 *	FOPS definition
 *************************************************************************/
static int cvi_dwa_queue_init(void *priv, struct vb2_queue *src_vq,
			       struct vb2_queue *dst_vq)
{
	struct cvi_dwa_ctx *ctx = priv;
	struct cvi_vip_dev *bdev = NULL;
	int rc = 0;

	bdev = container_of(ctx->wdev, struct cvi_vip_dev, dwa_vdev);

	memset(src_vq, 0, sizeof(*src_vq));
	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_USERPTR;
	src_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->min_buffers_needed = 0;
	src_vq->drv_priv = priv;
	src_vq->dev = bdev->v4l2_dev.dev;
	src_vq->ops = &cvi_dwa_qops;
	src_vq->mem_ops = &cvi_dwa_vb2_mem_ops;
	//q->lock = &wdev->lock;

	rc = vb2_queue_init(src_vq);
	if (rc) {
		dprintk(VIP_ERR, "src_vq errcode(%d)\n", rc);
		return rc;
	}

	memset(dst_vq, 0, sizeof(*dst_vq));
	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes = VB2_USERPTR;
	dst_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->min_buffers_needed = 0;
	dst_vq->drv_priv = priv;
	dst_vq->dev = bdev->v4l2_dev.dev;
	dst_vq->ops = &cvi_dwa_qops;
	dst_vq->mem_ops = &cvi_dwa_vb2_mem_ops;
	//q->lock = &wdev->lock;

	rc = vb2_queue_init(dst_vq);
	if (rc)
		dprintk(VIP_ERR, "dst_vq errcode(%d)\n", rc);

	return rc;
}

static int cvi_dwa_open(struct file *file)
{
	int rc = 0;
	struct cvi_dwa_vdev *wdev = video_drvdata(file);
	struct cvi_vip_dev *bdev = container_of(wdev, struct cvi_vip_dev, dwa_vdev);
	struct cvi_dwa_ctx *ctx = NULL;

	WARN_ON(!wdev);

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		rc = -ENOMEM;
		return rc;
	}

	if (mutex_lock_interruptible(&wdev->mutex)) {
		rc = -ERESTARTSYS;
		goto err_lock;
	}

	if (bdev->clk_sys[1])
		clk_prepare_enable(bdev->clk_sys[1]);
	if ((debug & BIT(2)) && wdev->clk)
		clk_prepare_enable(wdev->clk);

	v4l2_fh_init(&ctx->fh, &wdev->vdev);
	file->private_data = &ctx->fh;
	v4l2_fh_add(&ctx->fh);
	INIT_LIST_HEAD(&ctx->list);

	ctx->wdev = wdev;
	ctx->fh.m2m_ctx = v4l2_m2m_ctx_init(wdev->m2m_dev, ctx, cvi_dwa_queue_init);
	if (IS_ERR(ctx->fh.m2m_ctx)) {
		pr_err("Failed to initialize m2m context");
		rc = PTR_ERR(ctx->fh.m2m_ctx);
		goto err_m2m_ctx;
	}
	ctx->fmt = cvi_vip_get_format(V4L2_PIX_FMT_YUV420M);
	ctx->output = DWA_OUT_MEM;

	list_add(&ctx->list, &wdev->ctx_list);
	++wdev->ctx_num;
	mutex_unlock(&wdev->mutex);

	return rc;
err_m2m_ctx:
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
err_lock:
	kfree(ctx);
	return rc;
}

static int cvi_dwa_release(struct file *file)
{
	struct cvi_dwa_vdev *wdev = video_drvdata(file);
	struct cvi_vip_dev *bdev = container_of(wdev, struct cvi_vip_dev, dwa_vdev);
	struct cvi_dwa_ctx *ctx = fh_to_ctx(file->private_data);

	WARN_ON(!wdev);

	v4l2_m2m_streamoff(NULL, ctx->fh.m2m_ctx, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
	v4l2_m2m_streamoff(NULL, ctx->fh.m2m_ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);

	mutex_lock(&wdev->mutex);
	v4l2_m2m_ctx_release(ctx->fh.m2m_ctx);
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	file->private_data = NULL;
	--wdev->ctx_num;
	list_del_init(&ctx->list);
	mutex_unlock(&wdev->mutex);
	kfree(ctx);

	if (!(debug & BIT(2)) && wdev->clk && __clk_is_enabled(wdev->clk))
		clk_disable_unprepare(wdev->clk);
	if (!(debug & BIT(2)) && bdev->clk_sys[1] && __clk_is_enabled(bdev->clk_sys[1]))
		clk_disable_unprepare(bdev->clk_sys[1]);

	dprintk(VIP_INFO, "-\n");
	return 0;
}

static int cvi_dwa_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct cvi_dwa_vdev *wdev = video_drvdata(file);
	unsigned long vm_start = vma->vm_start;
	unsigned int vm_size = vma->vm_end - vma->vm_start;
	unsigned int offset = vma->vm_pgoff << PAGE_SHIFT;
	void *pos = wdev->shared_mem;

	if (offset < 0 || (vm_size + offset) > GDC_SHARE_MEM_SIZE)
		return -EINVAL;

	while (vm_size > 0) {
		if (remap_pfn_range(vma, vm_start, virt_to_pfn(pos), PAGE_SIZE, vma->vm_page_prot))
			return -EAGAIN;
		pr_debug("gdc proc mmap vir(%p) phys(%#llx)\n", pos, virt_to_phys((void *) pos));
		vm_start += PAGE_SIZE;
		pos += PAGE_SIZE;
		vm_size -= PAGE_SIZE;
	}

	return 0;
}

static struct v4l2_file_operations cvi_dwa_fops = {
	.owner          = THIS_MODULE,
	.open           = cvi_dwa_open,
	.release        = cvi_dwa_release,
	.poll           = v4l2_m2m_fop_poll,
	.unlocked_ioctl = video_ioctl2,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = video_ioctl2,
#endif
	.mmap           = cvi_dwa_mmap,
};
/*************************************************************************
 *	IOCTL definition
 *************************************************************************/
static int cvi_dwa_querycap(struct file *file, void *priv,
		    struct v4l2_capability *cap)
{
	struct cvi_dwa_vdev *wdev = video_drvdata(file);
	struct cvi_vip_dev *bdev =
		container_of(wdev, struct cvi_vip_dev, dwa_vdev);

	strlcpy(cap->driver, CVI_VIP_DRV_NAME, sizeof(cap->driver));
	strlcpy(cap->card, CVI_VIP_DVC_NAME, sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info),
	    "platform:%s", bdev->v4l2_dev.name);

	cap->capabilities = wdev->vid_caps | V4L2_CAP_DEVICE_CAPS;
	return 0;
}

static int cvi_dwa_s_ext_ctrls(struct file *file, void *priv,
	struct v4l2_ext_controls *vc)
{
	struct cvi_dwa_vdev *wdev = video_drvdata(file);
	struct cvi_dwa_ctx *ctx = fh_to_ctx(file->private_data);
	struct v4l2_ext_control *ext_ctrls;
	int rc = -EINVAL, i = 0;

	ext_ctrls = vc->controls;
	for (i = 0; i < vc->count; ++i) {
		switch (ext_ctrls[i].id) {
		case V4L2_CID_DV_VIP_DWA_SET_MESH:
			ctx->mesh_id_addr = ext_ctrls[i].value64;
			rc = 0;
			break;

		case V4L2_CID_DV_VIP_DWA_SET_OUTPUT:
			ctx->output = ext_ctrls[i].value;
			rc = 0;
			break;

		case V4L2_CID_DV_VIP_DWA_SET_BGCOLOR:
			ctx->bgcolor = ext_ctrls[i].value;
			rc = 0;
			break;

		case V4L2_CID_DV_VIP_DWA_SET_ALIGN:
			if (ext_ctrls[i].value >= VIP_ALIGNMENT) {
				wdev->align = ext_ctrls[i].value;
				rc = 0;
			}
			break;

		default:
			break;
		}
	}
	return rc;
}

int cvi_dwa_enum_fmt_vid_mplane(struct file *file, void  *priv,
		    struct v4l2_fmtdesc *f)
{
	dprintk(VIP_DBG, "%s+\n", V4L2_TYPE_IS_OUTPUT(f->type) ? "out" : "cap");
	return cvi_vip_enum_fmt_vid(file, priv, f);
}

int cvi_dwa_g_fmt_vid_mplane(struct file *file, void *priv,
		    struct v4l2_format *f)
{
	struct cvi_dwa_vdev *wdev = video_drvdata(file);
	struct cvi_dwa_ctx *ctx = fh_to_ctx(file->private_data);
	struct v4l2_pix_format_mplane *mp = &f->fmt.pix_mp;
	struct cvi_dwa_data *data = NULL;
	unsigned int p;

	dprintk(VIP_DBG, "%s+\n", V4L2_TYPE_IS_OUTPUT(f->type) ? "out" : "cap");
	WARN_ON(!wdev);

	data = _dwa_get_data(ctx, f->type);
	if (!data)
		return -EINVAL;

	mp->width        = data->w;
	mp->height       = data->h;
	mp->field        = V4L2_FIELD_NONE;
	mp->pixelformat  = ctx->fmt->fourcc;
	mp->colorspace   = ctx->colorspace;
	mp->xfer_func    = V4L2_XFER_FUNC_DEFAULT;
	mp->ycbcr_enc    = V4L2_YCBCR_ENC_DEFAULT;
	mp->quantization = V4L2_QUANTIZATION_DEFAULT;
	mp->num_planes   = ctx->fmt->buffers;
	for (p = 0; p < mp->num_planes; p++) {
		mp->plane_fmt[p].bytesperline = data->bytesperline[p];
		mp->plane_fmt[p].sizeimage = data->sizeimage[p];
	}

	return 0;
}

int cvi_dwa_try_fmt_vid_mplane(struct file *file, void *priv,
	    struct v4l2_format *f)
{
	struct cvi_dwa_vdev *wdev = video_drvdata(file);
	struct v4l2_pix_format_mplane *mp = &f->fmt.pix_mp;
	char buf[5];

	if ((mp->pixelformat != V4L2_PIX_FMT_YUV420M) && (mp->pixelformat != V4L2_PIX_FMT_RGBM)
	 && (mp->pixelformat != V4L2_PIX_FMT_GREY)) {
		dprintk(VIP_ERR, "Invalid format(%s), YUV420 or RGB planar or Y-only .\n",
			v4l2_fourcc2s(mp->pixelformat, buf));
		return -EINVAL;
	}
	return cvi_vip_try_fmt_vid_mplane(f, wdev->align);
}

int cvi_dwa_s_fmt_vid_mplane(struct file *file, void *priv,
	    struct v4l2_format *f)
{
	struct cvi_dwa_ctx *ctx = fh_to_ctx(file->private_data);
	struct v4l2_pix_format_mplane *mp = &f->fmt.pix_mp;
	struct v4l2_plane_pix_format *pfmt = mp->plane_fmt;
	const struct cvi_vip_fmt *fmt;
	unsigned int p;
	int rc = cvi_dwa_try_fmt_vid_mplane(file, priv, f);
	struct cvi_dwa_data *data = NULL;

	dprintk(VIP_DBG, "%s+\n", V4L2_TYPE_IS_OUTPUT(f->type) ? "out" : "cap");
	if (rc < 0)
		return rc;

	data = _dwa_get_data(ctx, f->type);
	if (!data)
		return -EINVAL;

	fmt = cvi_vip_get_format(mp->pixelformat);
	data->w = mp->width;
	data->h = mp->height;
	ctx->fmt = fmt;
	ctx->colorspace = mp->colorspace;
	for (p = 0; p < mp->num_planes; p++) {
		data->bytesperline[p] = pfmt[p].bytesperline;
		data->sizeimage[p] = pfmt[p].sizeimage;
	}

	return rc;
}

static const struct v4l2_ioctl_ops cvi_dwa_ioctl_ops = {
	.vidioc_querycap = cvi_dwa_querycap,
	.vidioc_s_ext_ctrls = cvi_dwa_s_ext_ctrls,

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	.vidioc_enum_fmt_vid_cap = cvi_dwa_enum_fmt_vid_mplane,
	.vidioc_enum_fmt_vid_out = cvi_dwa_enum_fmt_vid_mplane,
#else
	.vidioc_enum_fmt_vid_cap_mplane = cvi_dwa_enum_fmt_vid_mplane,
	.vidioc_enum_fmt_vid_out_mplane = cvi_dwa_enum_fmt_vid_mplane,
#endif
	.vidioc_g_fmt_vid_cap_mplane    = cvi_dwa_g_fmt_vid_mplane,
	.vidioc_g_fmt_vid_out_mplane    = cvi_dwa_g_fmt_vid_mplane,
	.vidioc_try_fmt_vid_cap_mplane  = cvi_dwa_try_fmt_vid_mplane,
	.vidioc_try_fmt_vid_out_mplane  = cvi_dwa_try_fmt_vid_mplane,
	.vidioc_s_fmt_vid_cap_mplane    = cvi_dwa_s_fmt_vid_mplane,
	.vidioc_s_fmt_vid_out_mplane    = cvi_dwa_s_fmt_vid_mplane,

	.vidioc_reqbufs         = v4l2_m2m_ioctl_reqbufs,
	.vidioc_create_bufs     = v4l2_m2m_ioctl_create_bufs,
	.vidioc_prepare_buf     = v4l2_m2m_ioctl_prepare_buf,
	.vidioc_querybuf        = v4l2_m2m_ioctl_querybuf,
	.vidioc_qbuf            = v4l2_m2m_ioctl_qbuf,
	.vidioc_dqbuf           = v4l2_m2m_ioctl_dqbuf,
	.vidioc_expbuf          = v4l2_m2m_ioctl_expbuf,
	.vidioc_streamon        = v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff       = v4l2_m2m_ioctl_streamoff,

};

/*************************************************************************
 *	General functions
 *************************************************************************/
int dwa_create_instance(struct platform_device *pdev)
{
	int rc = 0;
	struct cvi_vip_dev *bdev;
	struct video_device *vfd;
	struct cvi_dwa_vdev *wdev;
	u8 i = 0;

	bdev = dev_get_drvdata(&pdev->dev);
	if (!bdev) {
		dprintk(VIP_ERR, "invalid data\n");
		return -EINVAL;
	}

	wdev = &bdev->dwa_vdev;
	INIT_LIST_HEAD(&wdev->ctx_list);
	wdev->ctx_num = 0;
	mutex_init(&wdev->mutex);

	wdev->clk = devm_clk_get(&pdev->dev, CLK_DWA_NAME);
	if (IS_ERR(wdev->clk)) {
		pr_err("Cannot get clk for dwa\n");
		wdev->clk = NULL;
	}

	wdev->align = VIP_ALIGNMENT;
	wdev->m2m_dev = v4l2_m2m_init(&cvi_dwa_m2m_ops);
	if (IS_ERR(wdev->m2m_dev)) {
		dprintk(VIP_ERR, "Failed to init mem2mem device\n");
		rc = PTR_ERR(wdev->m2m_dev);
		goto err_m2m_init;
	}

	wdev->vid_caps = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING;

	vfd = &(wdev->vdev);
	snprintf(vfd->name, sizeof(vfd->name), "cvi-dwa");
	vfd->fops = &cvi_dwa_fops;
	vfd->ioctl_ops = &cvi_dwa_ioctl_ops;
	vfd->vfl_dir = VFL_DIR_M2M;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	vfd->vfl_type = VFL_TYPE_VIDEO;
#else
	vfd->vfl_type = VFL_TYPE_GRABBER;
#endif
	vfd->minor = -1;
	vfd->device_caps = wdev->vid_caps;
	vfd->release = video_device_release_empty;
	vfd->v4l2_dev = &bdev->v4l2_dev;
	vfd->lock = &wdev->mutex;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	rc = video_register_device(vfd, VFL_TYPE_VIDEO, DWA_DEVICE_IDX);
#else
	rc = video_register_device(vfd, VFL_TYPE_GRABBER, DWA_DEVICE_IDX);
#endif
	if (rc) {
		dprintk(VIP_ERR, "Failed to register img-device%d\n", i);
		goto err_dec_vdev_register;
	}

	video_set_drvdata(vfd, wdev);

	dprintk(VIP_INFO, "img registered as %s\n",
			video_device_node_name(vfd));

	wdev->shared_mem = kzalloc(GDC_SHARE_MEM_SIZE, GFP_ATOMIC);
	if (!wdev->shared_mem) {
		dprintk(VIP_ERR, "gdc shared mem alloc fail\n");
		return -ENOMEM;
	}

	if (gdc_proc_init(wdev->shared_mem) < 0)
		dprintk(VIP_ERR, "gdc proc init failed\n");

	return 0;

err_dec_vdev_register:
	v4l2_m2m_release(wdev->m2m_dev);
err_m2m_init:
	return rc;
}

int dwa_destroy_instance(struct platform_device *pdev)
{
	struct cvi_vip_dev *bdev;
	struct cvi_dwa_vdev *wdev;

	bdev = dev_get_drvdata(&pdev->dev);
	if (!bdev) {
		dprintk(VIP_ERR, "invalid data\n");
		return -EINVAL;
	}

	gdc_proc_remove();
	wdev = &bdev->dwa_vdev;
	kfree(wdev->shared_mem);

	return 0;
}

void dwa_irq_handler(u8 intr_status, struct cvi_vip_dev *bdev)
{
	struct cvi_dwa_vdev *wdev = NULL;
	struct vb2_v4l2_buffer *src_buf, *dst_buf;
	struct cvi_dwa_ctx *ctx;

	wdev = &bdev->dwa_vdev;
	ctx = v4l2_m2m_get_curr_priv(wdev->m2m_dev);

	hrtimer_cancel(&wdev->timer);
	if (!ctx || !ctx->fh.m2m_ctx)
		return;

	dprintk(VIP_DBG, "for m2m_ctx(%p)\n", ctx->fh.m2m_ctx);

	src_buf = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
	dst_buf = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);

	dst_buf->vb2_buf.timestamp = src_buf->vb2_buf.timestamp;
	dst_buf->timecode = src_buf->timecode;
	dst_buf->flags &= ~V4L2_BUF_FLAG_TSTAMP_SRC_MASK;
	dst_buf->flags |= src_buf->flags & V4L2_BUF_FLAG_TSTAMP_SRC_MASK;

	dwa_disable();

	v4l2_m2m_buf_done(src_buf, VB2_BUF_STATE_DONE);
	v4l2_m2m_buf_done(dst_buf, VB2_BUF_STATE_DONE);
	v4l2_m2m_job_finish(wdev->m2m_dev, ctx->fh.m2m_ctx);
}
