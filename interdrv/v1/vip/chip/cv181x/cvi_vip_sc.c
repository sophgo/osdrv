#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/videodev2.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-event.h>
#include <media/v4l2-rect.h>

#include "vip/vip_common.h"
#include "vip/scaler.h"

#include "cvi_debug.h"
#include "cvi_vip_core.h"
#include "cvi_vip_img.h"
#include "cvi_vip_sc.h"
#include "cvi_vip_vpss_proc.h"
#include "cvi_vip_rgn_proc.h"

#define VPSS_SHARE_MEM_SIZE     (0x8000)
#define SC_PROC_NAME "cvitek/vpss"

// for proc info
static int proc_vpss_mode;
static const char * const str_src[] = {"ISP", "DWA", "MEM"};
static const char * const str_sclr_flip[] = {"No", "HFLIP", "VFLIP", "HVFLIP"};
static const char * const str_sclr_odma_mode[] = {"CSC", "QUANT", "HSV", "DISABLE"};
static const char * const str_sclr_fmt[] = {"YUV420", "YUV422", "RGB_PLANAR", "RGB_PACKED", "BGR_PACKED", "Y_ONLY"};
static const char * const str_sclr_csc[] = {"Disable", "2RGB_601_Limit", "2RGB_601_Full", "2RGB_709_Limit"
	, "2RGB_709_Full", "2YUV_601_Limit", "2YUV_601_Full", "2YUV_709_Limit", "2YUV_709_Full"};

static const char *const CLK_SC_NAME[] = {"clk_sc_d", "clk_sc_v1", "clk_sc_v2", "clk_sc_v3"};

int cvi_sc_buf_queue(struct cvi_sc_vdev *vdev, struct cvi_sc_buf2 *list_buf)
{
	unsigned long flags;
	struct cvi_sc_buf2 *b = NULL;
	u8 grp_id = list_buf->buf.index;

	b = kmalloc(sizeof(struct cvi_sc_buf2), GFP_ATOMIC);
	if (b == NULL) {
		dprintk(VIP_ERR, "QBUF kmalloc size(%d) fail\n", sizeof(struct cvi_sc_buf2));
		return -1;
	}
	memcpy(b, list_buf, sizeof(struct cvi_sc_buf2));

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	list_add_tail(&b->list, &vdev->rdy_queue[grp_id]);
	++vdev->num_rdy[grp_id];
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);
	return 0;
}

struct cvi_sc_buf2 *cvi_sc_next_buf(struct cvi_sc_vdev *vdev, const u8 grp_id)
{
	unsigned long flags;
	struct cvi_sc_buf2 *b = NULL;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	if (!list_empty(&vdev->rdy_queue[grp_id]))
		b = list_first_entry(&vdev->rdy_queue[grp_id], struct cvi_sc_buf2, list);
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);

	return b;
}

int cvi_sc_buf_num(struct cvi_sc_vdev *vdev, const u8 grp_id)
{
	unsigned long flags;
	int num;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	num = vdev->num_rdy[grp_id];
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);

	return num;
}

int cvi_sc_buf_remove(struct cvi_sc_vdev *vdev, const u8 grp_id)
{
	unsigned long flags;
	struct cvi_sc_buf2 *b = NULL;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	if (vdev->num_rdy[grp_id] == 0) {
		spin_unlock_irqrestore(&vdev->rdy_lock, flags);
		return -1;
	}
	if (!list_empty(&vdev->rdy_queue[grp_id])) {
		b = list_first_entry(&vdev->rdy_queue[grp_id],
			struct cvi_sc_buf2, list);
		list_del_init(&b->list);
		kfree(b);
		--vdev->num_rdy[grp_id];
	}
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);

	return 0;
}

void cvi_sc_buf_remove_all(struct cvi_sc_vdev *vdev, const u8 grp_id)
{
	unsigned long flags;
	struct cvi_sc_buf2 *b = NULL;

	spin_lock_irqsave(&vdev->rdy_lock, flags);
	while (!list_empty(&vdev->rdy_queue[grp_id])) {
		b = list_first_entry(&vdev->rdy_queue[grp_id],
			struct cvi_sc_buf2, list);
		list_del_init(&b->list);
		kfree(b);
		--vdev->num_rdy[grp_id];
	}
	spin_unlock_irqrestore(&vdev->rdy_lock, flags);
}

void cvi_sc_device_run(void *priv, bool is_tile, bool is_work_on_r_tile, u8 grp_id)
{
	struct cvi_sc_vdev *sdev = priv;
	struct cvi_vip_buffer2 *b;
	struct cvi_sc_buf2 *list_buf = NULL;

	list_buf = cvi_sc_next_buf(sdev, grp_id);
	if (list_buf == NULL) {
		dprintk(VIP_ERR, "sc(%d) grp(%d) no buf\n", sdev->dev_idx, grp_id);
		return;
	}
	b = &list_buf->buf;
	if (!(debug & BIT(2)) && sdev->clk)
		clk_enable(sdev->clk);

	dprintk(VIP_DBG, "update sc-buf for grp(%d): 0x%llx-0x%llx-0x%llx\n", grp_id,
		b->planes[0].addr, b->planes[1].addr, b->planes[2].addr);

	sclr_odma_set_addr(sdev->dev_idx, b->planes[0].addr, b->planes[1].addr, b->planes[2].addr);

	if (!is_tile || !is_work_on_r_tile)
		cvi_sc_update(sdev, &sdev->vpss_chn_cfg[grp_id]);
}

/*************************************************************************
 *	VB2_OPS definition
 *************************************************************************/
#if 0
/**
 * call before VIDIOC_REQBUFS to setup buf-queue.
 * nbuffers: number of buffer requested
 * nplanes:  number of plane each buffer
 * sizes:    size of each plane(bytes)
 */
static int cvi_sc_queue_setup(struct vb2_queue *vq,
	       unsigned int *nbuffers, unsigned int *nplanes,
	       unsigned int sizes[], struct device *alloc_devs[])
{
	struct cvi_sc_vdev *sdev = vb2_get_drv_priv(vq);
	unsigned int planes = sdev->fmt->buffers;
	unsigned int p;

	dprintk(VIP_VB2, "(%d)+\n", sdev->dev_idx);

	// just give the min_length
	for (p = 0; p < planes; ++p)
		sizes[p] = sdev->sizeimage[p];

	if (vq->num_buffers + *nbuffers < 2)
		*nbuffers = 2 - vq->num_buffers;

	*nplanes = sdev->fmt->buffers;

	dprintk(VIP_INFO, "num_buffer=%d, num_plane=%d\n", *nbuffers, *nplanes);
	for (p = 0; p < *nplanes; p++)
		dprintk(VIP_INFO, "size[%u]=%u\n", p, sizes[p]);

	return 0;
}

/**
 * for VIDIOC_STREAMON, start fill data.
 */
static void cvi_sc_buf_queue(struct vb2_buffer *vb)
{
	struct cvi_sc_vdev *sdev = vb2_get_drv_priv(vb->vb2_queue);
	//struct cvi_vip_dev *bdev =
	//	container_of(sdev, struct cvi_vip_dev, sc_vdev[sdev->dev_idx]);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct cvi_vip_buffer *cvi_vb2 =
		container_of(vbuf, struct cvi_vip_buffer, vb);

	dprintk(VIP_VB2, "(%d)+ vb num_planes(%d)+\n", sdev->dev_idx, vb->num_planes);

	cvi_vip_buf_queue((struct cvi_base_vdev *)sdev, cvi_vb2);

	//cvi_vip_try_schedule(&bdev->img_vdev[sdev->img_src], 0);
}

static int cvi_sc_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct cvi_sc_vdev *sdev = vb2_get_drv_priv(vq);
	int rc = 0;

	dprintk(VIP_VB2, "(%d)+\n", sdev->dev_idx);

	sdev->seq_count = 0;
	return rc;
}

/* abort streaming and wait for last buffer */
static void cvi_sc_stop_streaming(struct vb2_queue *vq)
{
	struct cvi_sc_vdev *sdev = vb2_get_drv_priv(vq);
	struct cvi_vip_buffer *cvi_vb2, *tmp;
	unsigned long flags;
	struct vb2_buffer *vb2_buf;

	dprintk(VIP_VB2, "(%d)+\n", sdev->dev_idx);

	/*
	 * Release all the buffers enqueued to driver
	 * when streamoff is issued
	 */
	spin_lock_irqsave(&sdev->rdy_lock, flags);
	list_for_each_entry_safe(cvi_vb2, tmp, &(sdev->rdy_queue), list) {
		vb2_buf = &(cvi_vb2->vb.vb2_buf);
		if (vb2_buf->state == VB2_BUF_STATE_DONE)
			continue;
		vb2_buffer_done(vb2_buf, VB2_BUF_STATE_DONE);
	}
	sdev->num_rdy = 0;
	INIT_LIST_HEAD(&sdev->rdy_queue);
	spin_unlock_irqrestore(&sdev->rdy_lock, flags);
}

const struct vb2_ops cvi_sc_qops = {
//    .buf_init           =
	.queue_setup        = cvi_sc_queue_setup,
//    .buf_finish         = cvi_sc_buf_finish,
	.buf_queue          = cvi_sc_buf_queue,
	.start_streaming    = cvi_sc_start_streaming,
	.stop_streaming     = cvi_sc_stop_streaming,
//    .wait_prepare       = vb2_ops_wait_prepare,
//    .wait_finish        = vb2_ops_wait_finish,
};
#endif
/*************************************************************************
 *	VB2-MEM-OPS definition
 *************************************************************************/
#if 0
static void *sc_get_userptr(struct device *dev, unsigned long vaddr,
	unsigned long size, enum dma_data_direction dma_dir)
{
	return (void *)0xdeadbeef;
}

static void sc_put_userptr(void *buf_priv)
{
}

static const struct vb2_mem_ops cvi_sc_vb2_mem_ops = {
	.get_userptr = sc_get_userptr,
	.put_userptr = sc_put_userptr,
};
#endif
/*************************************************************************
 *	FOPS definition
 *************************************************************************/
static int cvi_sc_open(struct file *file)
{
	int rc = 0;
	struct cvi_sc_vdev *sdev = video_drvdata(file);

	WARN_ON(!sdev);

	rc = v4l2_fh_open(file);
	if (rc) {
		dprintk(VIP_ERR, "v4l2_fh_open failed(%d)\n", rc);
		return rc;
	}

	if (v4l2_fh_is_singular_file(file)) {
		struct cvi_vip_dev *bdev = container_of(sdev, struct cvi_vip_dev, sc_vdev[sdev->dev_idx]);
		u8 i = 0;

		if (bdev->clk_sys[1])
			clk_prepare_enable(bdev->clk_sys[1]);
		if (bdev->clk_sc_top)
			clk_prepare_enable(bdev->clk_sc_top);

		// temporarily enable clk for init.
		if (sdev->clk)
			clk_prepare_enable(sdev->clk);
		sclr_reg_shadow_sel(sdev->dev_idx, false);
		sclr_init(sdev->dev_idx);
		sclr_set_cfg(sdev->dev_idx, false, false, true, false);
		sclr_reg_force_up(sdev->dev_idx);
		if (!(debug & BIT(2)) && sdev->clk)
			clk_disable_unprepare(sdev->clk);

		for (i = 0; i < ISP_PRERAW_MAX; i++)
			atomic_set(&sdev->buf_empty[i], 0);
	}

	dprintk(VIP_INFO, "by %s\n", current->comm);
	return rc;
}

static int cvi_sc_release(struct file *file)
{
	struct cvi_sc_vdev *sdev = video_drvdata(file);

	WARN_ON(!sdev);

	if (v4l2_fh_is_singular_file(file)) {
		struct cvi_vip_dev *bdev = container_of(sdev, struct cvi_vip_dev, sc_vdev[sdev->dev_idx]);

		if (!(debug & BIT(2)) && sdev->clk && __clk_is_enabled(sdev->clk))
			clk_disable_unprepare(sdev->clk);
		if (!(debug & BIT(2)) && bdev->clk_sys[1] && __clk_is_enabled(bdev->clk_sys[1]))
			clk_disable_unprepare(bdev->clk_sys[1]);
		if (!(debug & BIT(2)) && bdev->clk_sc_top && __clk_is_enabled(bdev->clk_sc_top))
			clk_disable_unprepare(bdev->clk_sc_top);
	}

	v4l2_fh_release(file);

	cvi_sc_buf_remove_all(sdev, 0);
	cvi_sc_buf_remove_all(sdev, 1);
	dprintk(VIP_INFO, "-\n");
	return 0;
}

#if 0
static unsigned int cvi_sc_poll(struct file *file,
	struct poll_table_struct *wait)
{
	struct cvi_sc_vdev *sdev = video_drvdata(file);
	struct vb2_buffer *out_vb = NULL;
	unsigned long flags;
	int rc = 0;

	WARN_ON(!sdev);

	poll_wait(file, &sdev->vb_q.done_wq, wait);
	rc = vb2_fop_poll(file, wait);
	spin_lock_irqsave(&sdev->rdy_lock, flags);
	if (!list_empty(&sdev->vb_q.done_list))
	out_vb = list_first_entry(&sdev->vb_q.done_list, struct vb2_buffer,
				  done_entry);
	if (out_vb && (out_vb->state == VB2_BUF_STATE_DONE
		|| out_vb->state == VB2_BUF_STATE_ERROR))
	rc |= POLLOUT | POLLWRNORM;
	spin_unlock_irqrestore(&sdev->rdy_lock, flags);
	return rc;
}
#endif

static int cvi_sc_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct cvi_sc_vdev *sdev = video_drvdata(file);
	unsigned long vm_start = vma->vm_start;
	unsigned int vm_size = vma->vm_end - vma->vm_start;
	unsigned int offset = vma->vm_pgoff << PAGE_SHIFT;
	void *pos = sdev->shared_mem;

	if (offset < 0 || (vm_size + offset) > VPSS_SHARE_MEM_SIZE)
		return -EINVAL;

	while (vm_size > 0) {
		if (remap_pfn_range(vma, vm_start, virt_to_pfn(pos), PAGE_SIZE, vma->vm_page_prot))
			return -EAGAIN;
		pr_debug("vpss proc mmap vir(%p) phys(%#llx)\n", pos, virt_to_phys((void *) pos));
		vm_start += PAGE_SIZE;
		pos += PAGE_SIZE;
		vm_size -= PAGE_SIZE;
	}

	return 0;
}

static struct v4l2_file_operations cvi_sc_fops = {
	.owner = THIS_MODULE,
	.open = cvi_sc_open,
	.release = cvi_sc_release,
	.mmap = cvi_sc_mmap,
	.poll = vb2_fop_poll, // .poll = cvi_sc_poll,
	.unlocked_ioctl = video_ioctl2,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = video_ioctl2,
#endif
};

/*************************************************************************
 *	IOCTL definition
 *************************************************************************/
static int cvi_sc_querycap(struct file *file, void *priv,
		    struct v4l2_capability *cap)
{
	struct cvi_sc_vdev *sdev = video_drvdata(file);
	struct cvi_vip_dev *bdev =
		container_of(sdev, struct cvi_vip_dev, sc_vdev[sdev->dev_idx]);

	strlcpy(cap->driver, CVI_VIP_DRV_NAME, sizeof(cap->driver));
	strlcpy(cap->card, CVI_VIP_DVC_NAME, sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info),
	    "platform:%s", bdev->v4l2_dev.name);

	cap->capabilities = sdev->vid_caps | V4L2_CAP_DEVICE_CAPS;
	return 0;
}

static int cvi_sc_g_ctrl(struct file *file, void *priv, struct v4l2_control *vc)
{
	int rc = -EINVAL;
	return rc;
}

static int cvi_sc_s_ctrl(struct file *file, void *priv, struct v4l2_control *vc)
{
	int rc = -EINVAL;
	return rc;
}

static int _sc_ext_set_sc_d_to_img_v(struct cvi_sc_vdev *sdev, u8 enable)
{
	struct sclr_top_cfg *cfg = NULL;
	struct cvi_vip_dev *bdev =
		container_of(sdev, struct cvi_vip_dev, sc_vdev[sdev->dev_idx]);

	if (sdev->dev_idx != 0) {
		dprintk(VIP_ERR,
			"sc-(%d) invalid for SC-D set img_src.\n",
			sdev->dev_idx);
		return -EINVAL;
	}

	if (enable >= 2) {
		dprintk(VIP_ERR, "invalid parameter(%d).\n", enable);
		return -EINVAL;
	}

	cfg = sclr_top_get_cfg();
	cfg->sclr_d_src = enable;
	sclr_top_set_cfg(cfg);

	sdev->img_src = enable;
	if (enable == 0) {
		bdev->img_vdev[CVI_VIP_IMG_D].sc_bounding = CVI_VIP_IMG_2_SC_D;
		bdev->img_vdev[CVI_VIP_IMG_V].sc_bounding = CVI_VIP_IMG_2_SC_V;
	} else {
		bdev->img_vdev[CVI_VIP_IMG_D].sc_bounding = CVI_VIP_IMG_2_SC_NONE;
		bdev->img_vdev[CVI_VIP_IMG_V].sc_bounding = CVI_VIP_IMG_2_SC_ALL;
	}

	return 0;
}

static int _sc_ext_set_quant(struct cvi_sc_vdev *sdev, const struct cvi_sc_quant_param *param)
{
	struct cvi_vip_dev *bdev =
		container_of(sdev, struct cvi_vip_dev, sc_vdev[sdev->dev_idx]);
	struct sclr_odma_cfg *odma_cfg = sclr_odma_get_cfg(sdev->dev_idx);

	if (!param->enable)
		return 0;

	memcpy(odma_cfg->csc_cfg.quant_form.sc_frac, param->sc_frac, sizeof(param->sc_frac));
	memcpy(odma_cfg->csc_cfg.quant_form.sub, param->sub, sizeof(param->sub));
	memcpy(odma_cfg->csc_cfg.quant_form.sub_frac, param->sub_frac, sizeof(param->sub_frac));

	odma_cfg->csc_cfg.mode = SCL_OUT_QUANT;
	odma_cfg->csc_cfg.quant_round = param->rounding;
	odma_cfg->csc_cfg.work_on_border = false;

	sclr_ctrl_set_output(sdev->dev_idx, &odma_cfg->csc_cfg, sdev->fmt->fmt);

	// if fmt is yuv, try use img'csc to convert rgb to yuv.
	if (IS_YUV_FMT(odma_cfg->fmt)) {
		struct cvi_img_vdev *idev = &bdev->img_vdev[sdev->img_src];
		struct sclr_img_cfg *img_cfg = sclr_img_get_cfg(idev->img_type);
		enum sclr_input input;

		img_cfg->csc = (IS_YUV_FMT(img_cfg->fmt))
			     ? SCL_CSC_NONE : SCL_CSC_601_LIMIT_RGB2YUV;

		if (idev->is_online_from_isp) {
			dprintk(VIP_ERR, "quant for yuv not work in online.\n");
			return -EINVAL;
		}
		cvi_img_get_input(idev->img_type, idev->input_type, &input);
		sclr_ctrl_set_input(idev->img_type, input, img_cfg->fmt, img_cfg->csc,
			idev->input_type == CVI_VIP_INPUT_ISP);
	}

	return 0;
}

static int _sc_ext_set_border(struct cvi_sc_vdev *sdev, const struct cvi_sc_border_param *param)
{
	struct sclr_border_cfg cfg;
	struct sclr_odma_cfg *odma_cfg;

	if (param->enable) {
		// full-size odma for border
		odma_cfg = sclr_odma_get_cfg(sdev->dev_idx);
		odma_cfg->mem.start_x = odma_cfg->mem.start_y = 0;
		odma_cfg->mem.width = odma_cfg->frame_size.w;
		odma_cfg->mem.height = odma_cfg->frame_size.h;
		sclr_odma_set_mem(sdev->dev_idx, &odma_cfg->mem);
	}

	cfg.cfg.b.enable = param->enable;
	cfg.cfg.b.bd_color_r = param->bg_color[0];
	cfg.cfg.b.bd_color_g = param->bg_color[1];
	cfg.cfg.b.bd_color_b = param->bg_color[2];
	cfg.start.x = param->offset_x;
	cfg.start.y = param->offset_y;
	sclr_border_set_cfg(sdev->dev_idx, &cfg);

	return 0;
}

static int cvi_sc_s_ext_ctrls(struct file *file, void *priv,
	struct v4l2_ext_controls *vc)
{
	struct v4l2_ext_control *ext_ctrls;
	int rc = -EINVAL, i = 0;
	struct cvi_sc_vdev *sdev = video_drvdata(file);

	ext_ctrls = vc->controls;
	for (i = 0; i < vc->count; ++i) {
		switch (ext_ctrls[i].id) {
		case V4L2_CID_DV_VIP_SC_SET_SC_D_SRC_TO_V:
			rc = _sc_ext_set_sc_d_to_img_v(sdev, ext_ctrls[i].value);
		break;

		case V4L2_CID_DV_VIP_SC_SET_FLIP:
			sdev->vpss_chn_cfg[0].flip = ext_ctrls[i].value;
			rc = 0;
		break;

		case V4L2_CID_DV_VIP_SC_SET_QUANT: {
			if (copy_from_user(&sdev->vpss_chn_cfg[0].quant_cfg, ext_ctrls[i].ptr,
				sizeof(sdev->vpss_chn_cfg[0].quant_cfg))) {
				dprintk(VIP_ERR, "ioctl-%#x, copy_from_user failed.\n", ext_ctrls[i].id);
				rc = -ENOMEM;
				break;
			}
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_SC_SET_BORDER: {
			if (copy_from_user(&sdev->vpss_chn_cfg[0].border_cfg, ext_ctrls[i].ptr,
				sizeof(sdev->vpss_chn_cfg[0].border_cfg))) {
				dprintk(VIP_ERR, "ioctl-%#x, copy_from_user failed.\n", ext_ctrls[i].id);
				rc = -ENOMEM;
				break;
			}
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_SC_SET_ALIGN:
			if (ext_ctrls[i].value >= VIP_ALIGNMENT) {
				sdev->align = ext_ctrls[i].value;
				rc = 0;
			}
		break;

		case V4L2_CID_DV_VIP_SC_SET_RGN: {
			if (copy_from_user(&sdev->vpss_chn_cfg[0].rgn_cfg, ext_ctrls[i].ptr,
				sizeof(sdev->vpss_chn_cfg[0].rgn_cfg))) {
				dprintk(VIP_ERR, "ioctl-%#x, copy_from_user failed.\n", ext_ctrls[i].id);
				rc = -ENOMEM;
				break;
			}
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_SC_SET_COEF: {
			if (ext_ctrls[i].value < CVI_SC_SCALING_COEF_MAX) {
				sdev->vpss_chn_cfg[0].sc_coef = ext_ctrls[i].value;
				rc = 0;
			}
		}
		break;

		case V4L2_CID_DV_VIP_SC_SET_CUSTOM_CSC: {
			if (copy_from_user(&sdev->vpss_chn_cfg[0].csc_cfg, ext_ctrls[i].ptr,
				sizeof(sdev->vpss_chn_cfg[0].csc_cfg))) {
				dprintk(VIP_ERR, "ioctl-%#x, copy_from_user failed.\n", ext_ctrls[i].id);
				rc = -ENOMEM;
				break;
			}
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_SC_SET_VPSS_CHN_CFG: {
			struct cvi_vpss_chn_cfg cfg;

			if (copy_from_user(&cfg, ext_ctrls[i].ptr, sizeof(cfg))) {
				dprintk(VIP_ERR, "ioctl-%#x, copy_from_user failed.\n", ext_ctrls[i].id);
				rc = -ENOMEM;
				break;
			}
			sdev->vpss_chn_cfg[cfg.grp_id] = cfg;
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_QBUF: {
			struct cvi_vip_buffer2 buf;
			struct cvi_vip_dev *bdev =
				container_of(sdev, struct cvi_vip_dev, sc_vdev[sdev->dev_idx]);
			struct cvi_img_vdev *idev = &bdev->img_vdev[sdev->img_src];
			struct cvi_sc_buf2 list_buf;

			if (copy_from_user(&buf, ext_ctrls[i].ptr, sizeof(buf))) {
				dprintk(VIP_ERR, "ioctl-%#x, copy_from_user failed.\n", ext_ctrls[i].id);
				rc = -ENOMEM;
				break;
			}

			if (idev->is_online_from_isp) {
				if (buf.index > 1)
					buf.index = 0;
				if (cvi_sc_buf_num(sdev, buf.index) > 1) {
					dprintk(VIP_ERR, "sc(%d): previous buf_index(%d) isn't finished yet\n",
						sdev->dev_idx, buf.index);
					break;
				}
			} else {
				buf.index = 0;
				if (cvi_sc_buf_num(sdev, buf.index)) {
					dprintk(VIP_ERR, "sc(%d): previous buf_index(%d) isn't finished yet\n",
						sdev->dev_idx, buf.index);
					break;
				}
			}

			list_buf.buf = buf;
			if (cvi_sc_buf_queue(sdev, &list_buf))
				break;
			if (idev->is_online_from_isp && (atomic_cmpxchg(&sdev->buf_empty[buf.index], 1, 0) == 1))
				cvi_sc_qbuf_trigger_str(bdev);

			dprintk(VIP_INFO, "sc(%d): num_rdy(%d) buf_index(%d) num_buffer(%d) addr(%#llx %#llx %#llx)\n",
				sdev->dev_idx, sdev->num_rdy[buf.index], buf.index, buf.length,
				buf.planes[0].addr, buf.planes[1].addr, buf.planes[2].addr);
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_SC_SET_BIND_FB: {
			sdev->bind_fb = ext_ctrls[i].value;
			rc = 0;
		}
		break;

		default:
		break;
		}
	}
	return rc;
}

int cvi_sc_g_selection(struct file *file, void *priv, struct v4l2_selection *sel)
{
	struct cvi_sc_vdev *sdev = video_drvdata(file);
	struct cvi_vip_dev *bdev =
		container_of(sdev, struct cvi_vip_dev, sc_vdev[sdev->dev_idx]);
	int rc = -EINVAL;

	dprintk(VIP_DBG, "(%d)+\n", sdev->dev_idx);

	sel->r.left = sel->r.top = 0;
	switch (sel->target) {
	case V4L2_SEL_TGT_CROP_BOUNDS:
	case V4L2_SEL_TGT_CROP_DEFAULT:
		sel->r.top = sel->r.left = 0;
		sel->r.width = bdev->img_vdev[sdev->img_src].crop_rect.width;
		sel->r.height = bdev->img_vdev[sdev->img_src].crop_rect.height;
		rc = 0;
	break;

	case V4L2_SEL_TGT_CROP:
		sel->r = sdev->crop_rect;
		rc = 0;
	break;

	case V4L2_SEL_TGT_COMPOSE:
		sel->r = sdev->compose_out;
		rc = 0;
	break;

	case V4L2_SEL_TGT_COMPOSE_DEFAULT:
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
		sel->r = sdev->sink_rect;
		rc = 0;
	break;

	default:
		return rc;
	}

	dprintk(VIP_INFO, "target(%d) rect(%d %d %d %d)\n", sel->target,
			sel->r.left, sel->r.top, sel->r.width, sel->r.height);
	return rc;
}

int cvi_sc_s_selection(struct file *file, void *fh, struct v4l2_selection *sel)
{
	struct cvi_sc_vdev *sdev = video_drvdata(file);
	struct cvi_vip_dev *bdev = container_of(sdev, struct cvi_vip_dev, sc_vdev[sdev->dev_idx]);
	int rc = -EINVAL;

	dprintk(VIP_DBG, "(%d)+\n", sdev->dev_idx);

	if (cvi_vip_job_is_queued(&bdev->img_vdev[sdev->img_src])) {
		dprintk(VIP_ERR, "job in queue\n");
		rc = -EAGAIN;
		return rc;
	}

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP_BOUNDS:
	{
		sdev->vpss_chn_cfg[0].src_size.width = sel->r.width;
		sdev->vpss_chn_cfg[0].src_size.height = sel->r.height;
		rc = 0;
	}
	break;
	case V4L2_SEL_TGT_CROP: {
		sdev->vpss_chn_cfg[0].crop = sel->r;
		sdev->crop_rect = sel->r;
	}
	rc = 0;
	break;

	case V4L2_SEL_TGT_COMPOSE: {
		sdev->vpss_chn_cfg[0].dst_rect = sel->r;
		sdev->compose_out = sel->r;
	}
	rc = 0;
	break;
	default:
		return rc;
	}

	dprintk(VIP_INFO, "target(%d) rect(%d %d %d %d)\n", sel->target,
			sel->r.left, sel->r.top, sel->r.width, sel->r.height);
	return rc;
}

int cvi_sc_enum_fmt_vid_mplane(struct file *file, void  *priv,
		    struct v4l2_fmtdesc *f)
{
	dprintk(VIP_DBG, "+\n");
	return cvi_vip_enum_fmt_vid(file, priv, f);
}

int cvi_sc_g_fmt_vid_out_mplane(struct file *file, void *priv,
		    struct v4l2_format *f)
{
	struct cvi_sc_vdev *sdev = video_drvdata(file);
	struct v4l2_pix_format_mplane *mp = &f->fmt.pix_mp;
	unsigned int p;

	dprintk(VIP_DBG, "(%d)+\n", sdev->dev_idx);
	WARN_ON(!sdev);

	mp->width        = sdev->compose_out.width;
	mp->height       = sdev->compose_out.height;
	mp->field        = V4L2_FIELD_NONE;
	mp->pixelformat  = sdev->fmt->fourcc;
	mp->colorspace   = sdev->colorspace;
	mp->xfer_func    = V4L2_XFER_FUNC_DEFAULT;
	mp->ycbcr_enc    = V4L2_YCBCR_ENC_DEFAULT;
	mp->quantization = V4L2_QUANTIZATION_DEFAULT;
	mp->num_planes   = sdev->fmt->buffers;
	for (p = 0; p < mp->num_planes; p++) {
		mp->plane_fmt[p].bytesperline = sdev->bytesperline[p];
		mp->plane_fmt[p].sizeimage = sdev->sizeimage[p];
	}

	return 0;
}

int cvi_sc_try_fmt_vid_out_mplane(struct file *file, void *priv,
	    struct v4l2_format *f)
{
	struct cvi_sc_vdev *sdev = video_drvdata(file);

	return cvi_vip_try_fmt_vid_mplane(f, sdev->align);
}

int cvi_sc_s_fmt_vid_out_mplane(struct file *file, void *priv,
	    struct v4l2_format *f)
{
	struct cvi_sc_vdev *sdev = video_drvdata(file);
	struct v4l2_pix_format_mplane *mp = &f->fmt.pix_mp;
	struct v4l2_plane_pix_format *pfmt = mp->plane_fmt;
	const struct cvi_vip_fmt *fmt;
	unsigned int p;
	int rc = cvi_sc_try_fmt_vid_out_mplane(file, priv, f);

	dprintk(VIP_DBG, "(%d)+\n", sdev->dev_idx);
	if (rc < 0)
		return rc;
	//TODO: check if job queue

	fmt = cvi_vip_get_format(mp->pixelformat);
	sdev->fmt = fmt;
	sdev->colorspace = mp->colorspace;
	for (p = 0; p < mp->num_planes; p++) {
		sdev->bytesperline[p] = pfmt[p].bytesperline;
		sdev->sizeimage[p] = pfmt[p].sizeimage;
	}
	sdev->sink_rect.width = mp->width;
	sdev->sink_rect.height = mp->height;
	dprintk(VIP_INFO, "sc(%d) sink size(%d-%d) num_planes(%d)\n", sdev->dev_idx,
		sdev->sink_rect.width, sdev->sink_rect.height, mp->num_planes);

	sdev->vpss_chn_cfg[0].src_size.width = mp->width;
	sdev->vpss_chn_cfg[0].src_size.height = mp->width;
	sdev->vpss_chn_cfg[0].pixelformat = mp->pixelformat;
	sdev->vpss_chn_cfg[0].bytesperline[0] = pfmt[0].bytesperline;
	sdev->vpss_chn_cfg[0].bytesperline[1] = pfmt[1].bytesperline;
	sdev->vpss_chn_cfg[0].crop.left = 0;
	sdev->vpss_chn_cfg[0].crop.top = 0;
	sdev->vpss_chn_cfg[0].crop.width = mp->width;
	sdev->vpss_chn_cfg[0].crop.height = mp->height;
	sdev->vpss_chn_cfg[0].dst_size.width = mp->width;
	sdev->vpss_chn_cfg[0].dst_size.height = mp->height;
	return rc;
}

int cvi_sc_streamon(struct file *file, void *fh, enum v4l2_buf_type i)
{
	struct cvi_sc_vdev *sdev = video_drvdata(file);
	int rc = 0;

	if (atomic_cmpxchg(&sdev->is_streaming, 0, 1) != 0) {
		dprintk(VIP_DBG, "sc(%d) is running\n", sdev->dev_idx);
		return rc;
	}

	//rc = vb2_streamon(&sdev->vb_q, i);
	if (!(debug & BIT(2)) && sdev->clk)
		clk_prepare(sdev->clk);

	return rc;
}

int cvi_sc_streamoff(struct file *file, void *fh, enum v4l2_buf_type i)
{
	struct cvi_sc_vdev *sdev = video_drvdata(file);
	int rc = 0, count = 10;

	if (atomic_cmpxchg(&sdev->is_streaming, 1, 0) != 1) {
		dprintk(VIP_DBG, "sc(%d) is off\n", sdev->dev_idx);
		return rc;
	}

	while (--count > 0) {
		if (atomic_read(&sdev->job_state) == CVI_VIP_IDLE)
			break;
		dprintk(VIP_DBG, "wait count(%d)\n", count);
		usleep_range(5 * 1000, 10 * 1000);
	}

	if (count == 0) {
		struct sclr_status sts = sclr_get_status(sdev->dev_idx);

		dprintk(VIP_ERR, "sc(%d) isn't idle. crop(%d) hscale(%d) vscale(%d) gop(%d) dma(%d)\n",
			sdev->dev_idx, sts.crop_idle, sts.hscale_idle, sts.vscale_idle, sts.gop_idle, sts.wdma_idle);

		sclr_img_reset(sdev->img_src);
	}
	if (!(debug & BIT(2)) && sdev->clk)
		while (__clk_is_enabled(sdev->clk))
			clk_disable(sdev->clk);

	if (!(debug & BIT(2)) && sdev->clk)
		clk_unprepare(sdev->clk);

	cvi_sc_buf_remove_all(sdev, 0);
	cvi_sc_buf_remove_all(sdev, 1);
	atomic_set(&sdev->job_state, CVI_VIP_IDLE);

	//rc = vb2_streamoff(&sdev->vb_q, i);
	return rc;
}

static const struct v4l2_ioctl_ops cvi_sc_ioctl_ops = {
	.vidioc_querycap = cvi_sc_querycap,
	.vidioc_g_ctrl = cvi_sc_g_ctrl,
	.vidioc_s_ctrl = cvi_sc_s_ctrl,
	.vidioc_s_ext_ctrls = cvi_sc_s_ext_ctrls,

	.vidioc_g_selection     = cvi_sc_g_selection,
	.vidioc_s_selection     = cvi_sc_s_selection,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	.vidioc_enum_fmt_vid_out = cvi_sc_enum_fmt_vid_mplane,
#else
	.vidioc_enum_fmt_vid_out_mplane = cvi_sc_enum_fmt_vid_mplane,
#endif
	.vidioc_g_fmt_vid_out_mplane    = cvi_sc_g_fmt_vid_out_mplane,
	.vidioc_try_fmt_vid_out_mplane  = cvi_sc_try_fmt_vid_out_mplane,
	.vidioc_s_fmt_vid_out_mplane    = cvi_sc_s_fmt_vid_out_mplane,

	.vidioc_reqbufs         = vb2_ioctl_reqbufs,
	//.vidioc_create_bufs     = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf     = vb2_ioctl_prepare_buf,
	.vidioc_querybuf        = vb2_ioctl_querybuf,
	.vidioc_qbuf            = vb2_ioctl_qbuf,
	.vidioc_dqbuf           = vb2_ioctl_dqbuf,
	//.vidioc_expbuf          = vb2_ioctl_expbuf,
	.vidioc_streamon        = cvi_sc_streamon,
	.vidioc_streamoff       = cvi_sc_streamoff,

	//.vidioc_subscribe_event     = sc_subscribe_event,
	//.vidioc_unsubscribe_event   = v4l2_event_unsubscribe,
};

/*************************************************************************
 *	Proc functions
 *************************************************************************/
static void _show_mem(struct seq_file *m, struct sclr_mem *mem)
{
	seq_printf(m, "start_x(%3d)\t\tstart_y(%3d)\t\twidth(%4d)\t\theight(%4d)\n"
		  , mem->start_x, mem->start_y, mem->width, mem->height);
	seq_printf(m, "pitch_y(%3d)\t\tpitch_c(%3d)\n", mem->pitch_y, mem->pitch_c);
}

static void _show_img_status(struct seq_file *m, u8 i)
{
	struct sclr_img_cfg *cfg = sclr_img_get_cfg(i);
	union sclr_img_dbg_status status = sclr_img_get_dbg_status(i, true);

	seq_printf(m, "--------------IMG_IN%d STATUS-----------------\n", i);
	seq_printf(m, "src(%s)\t\tcsc(%15s)\tfmt(%s)\t\tburst(%d)\n", str_src[cfg->src], str_sclr_csc[cfg->csc]
		  , str_sclr_fmt[cfg->fmt], cfg->burst);
	_show_mem(m, &cfg->mem);
	seq_printf(m, "err_fwr_yuv(%d%d%d)\terr_erd_yuv(%d%d%d)\tlb_full_yuv(%d%d%d)\tlb_empty_yuv(%d%d%d)\n"
		  , status.b.err_fwr_y, status.b.err_fwr_u,  status.b.err_fwr_v, status.b.err_erd_y
		  , status.b.err_erd_u, status.b.err_erd_v, status.b.lb_full_y, status.b.lb_full_u
		  , status.b.lb_full_v, status.b.lb_empty_y, status.b.lb_empty_u, status.b.lb_empty_v);
	seq_printf(m, "ip idle(%d)\t\tip int(%d)\n", status.b.ip_idle, status.b.ip_int);
}

static void _show_sc_status(struct seq_file *m, u8 i)
{
	struct sclr_core_cfg *cfg = sclr_get_cfg(i);
	struct sclr_odma_cfg *odma_cfg = sclr_odma_get_cfg(i);
	union sclr_odma_dbg_status status = sclr_odma_get_dbg_status(i);

	seq_printf(m, "--------------SC%d STATUS---------------------\n", i);
	seq_printf(m, "sc bypass(%d)\t\tgop bypass(%d)\t\tcir bypass(%d)\t\todma bypass(%d)\n"
		  , cfg->sc_bypass, cfg->gop_bypass, cfg->cir_bypass, cfg->odma_bypass);
	seq_printf(m, "src-size(%4d*%4d)\tcrop offset(%4d*%4d)\tcrop size(%4d*%4d)\toutput size(%4d*%4d)\n"
		  , cfg->sc.src.w, cfg->sc.src.h, cfg->sc.crop.x, cfg->sc.crop.y, cfg->sc.crop.w, cfg->sc.crop.h
		  , cfg->sc.dst.w, cfg->sc.dst.h);
	seq_printf(m, "flip(%s)\t\tfmt(%s)\t\tburst(%d)\n", str_sclr_flip[odma_cfg->flip]
		  , str_sclr_fmt[odma_cfg->fmt], odma_cfg->burst);
	seq_printf(m, "mode(%s)\t\tcsc(%15s)\t\n", str_sclr_odma_mode[odma_cfg->csc_cfg.mode]
		  , str_sclr_csc[odma_cfg->csc_cfg.csc_type]);
	_show_mem(m, &odma_cfg->mem);
	seq_printf(m, "full_yuv(%d%d%d)\t\tempty_yuv(%d%d%d)\t\taxi_active_yuv(%d%d%d)\taxi_active(%d)\n"
		  , status.b.y_buf_full, status.b.u_buf_full, status.b.v_buf_full
		  , status.b.y_buf_empty, status.b.u_buf_empty, status.b.v_buf_empty
		  , status.b.y_axi_active, status.b.u_axi_active, status.b.v_axi_active, status.b.axi_active);
}

static int sc_proc_show(struct seq_file *m, void *v)
{
	u8 i;

	// show driver status if vpss_mode == 1
	if (proc_vpss_mode) {
		for (i = 0; i < CVI_VIP_IMG_MAX; ++i)
			_show_img_status(m, i);
		for (i = 0; i < CVI_VIP_SC_MAX; ++i)
			_show_sc_status(m, i);
		return 0;
	}
	vpss_proc_show(m, v);

	return 0;
}

static ssize_t sc_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
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

	if (kstrtoint(cProcInputdata, 10, &proc_vpss_mode))
		proc_vpss_mode = 0;

	return count;
}

static int sc_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sc_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops sc_proc_fops = {
	.proc_open = sc_proc_open,
	.proc_read = seq_read,
	.proc_write = sc_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations sc_proc_fops = {
	.owner = THIS_MODULE,
	.open = sc_proc_open,
	.read = seq_read,
	.write = sc_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

/*************************************************************************
 *	General functions
 *************************************************************************/
int sc_create_instance(struct platform_device *pdev)
{
	int rc = 0;
	struct cvi_vip_dev *bdev;
	struct video_device *vfd;
	struct cvi_sc_vdev *sdev;
	//struct vb2_queue *q;
	u8 i = 0;

	bdev = dev_get_drvdata(&pdev->dev);
	if (!bdev) {
		dprintk(VIP_ERR, "invalid data\n");
		return -EINVAL;
	}

	for (i = 0; i < CVI_VIP_SC_MAX; ++i) {
		u8 j = 0;

		sdev = &bdev->sc_vdev[i];
		mutex_init(&sdev->mutex);

		sdev->clk = devm_clk_get(&pdev->dev, CLK_SC_NAME[i]);
		if (IS_ERR(sdev->clk)) {
			pr_err("Cannot get clk for sc-%d\n", i);
			sdev->clk = NULL;
		}

		sdev->align = VIP_ALIGNMENT;
		sdev->dev_idx = i;
		sdev->fmt = cvi_vip_get_format(V4L2_PIX_FMT_RGBM);
		sdev->vid_caps = V4L2_CAP_VIDEO_OUTPUT_MPLANE | V4L2_CAP_STREAMING;
		sdev->img_src = (i == 0) ? CVI_VIP_IMG_D : CVI_VIP_IMG_V;
		sdev->sc_coef = CVI_SC_SCALING_COEF_BICUBIC;
		sdev->tile_mode = SCL_TILE_MAX;
		sdev->is_cmdq = false;
		spin_lock_init(&sdev->rdy_lock);
		atomic_set(&sdev->job_state, CVI_VIP_IDLE);
		atomic_set(&sdev->is_streaming, 0);
		for (j = 0; j < ISP_PRERAW_MAX; j++)
			atomic_set(&sdev->buf_empty[i], 0);
		memset(&sdev->crop_rect, 0, sizeof(sdev->crop_rect));
		memset(&sdev->compose_out, 0, sizeof(sdev->compose_out));
		memset(&sdev->sink_rect, 0, sizeof(sdev->sink_rect));

		vfd = &(sdev->vdev);
		snprintf(vfd->name, sizeof(vfd->name), "cvi-sc%d", i);
		vfd->fops = &cvi_sc_fops;
		vfd->ioctl_ops = &cvi_sc_ioctl_ops;
		vfd->vfl_dir = VFL_DIR_TX;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		vfd->vfl_type = VFL_TYPE_VIDEO;
#else
		vfd->vfl_type = VFL_TYPE_GRABBER;
#endif
		vfd->minor = -1;
		vfd->device_caps = sdev->vid_caps;
		vfd->release = video_device_release_empty;
		vfd->v4l2_dev = &bdev->v4l2_dev;
		vfd->lock = &sdev->mutex;
#if 0
		vfd->queue = &sdev->vb_q;

		// vb2_queue init
		q = &sdev->vb_q;
		q->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		q->io_modes = VB2_USERPTR;
		q->buf_struct_size = sizeof(struct cvi_vip_buffer);
		q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
		q->min_buffers_needed = 0;
		q->drv_priv = sdev;
		q->dev = bdev->v4l2_dev.dev;
		q->ops = &cvi_sc_qops;
		q->mem_ops = &cvi_sc_vb2_mem_ops;
		//q->lock = &sdev->lock;
		rc = vb2_queue_init(q);
		if (rc) {
			dprintk(VIP_ERR, "errcode(%d)\n", rc);
			continue;
		}
		INIT_LIST_HEAD(&sdev->rdy_queue);
#endif
		sdev->num_rdy[0] = sdev->num_rdy[1] = 0;
		INIT_LIST_HEAD(&sdev->rdy_queue[0]);
		INIT_LIST_HEAD(&sdev->rdy_queue[1]);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		rc = video_register_device(vfd, VFL_TYPE_VIDEO,
				SC_DEVICE_IDX + i);
#else
		rc = video_register_device(vfd, VFL_TYPE_GRABBER,
				SC_DEVICE_IDX + i);
#endif
		if (rc) {
			dprintk(VIP_ERR, "Failed to register sc-device%d\n", i);
			continue;
		}
		video_set_drvdata(vfd, sdev);

		dprintk(VIP_INFO, "sc registered as %s\n",
				video_device_node_name(vfd));

		if (i == 0) {
			sdev->shared_mem = kzalloc(VPSS_SHARE_MEM_SIZE, GFP_ATOMIC);
			if (!sdev->shared_mem) {
				dprintk(VIP_ERR, "vpss shared mem alloc fail\n");
				return -ENOMEM;
			}

			if (vpss_proc_init(sdev->shared_mem) < 0)
				dprintk(VIP_ERR, "vpss proc init failed\n");

			if (proc_create_data(SC_PROC_NAME, 0644, NULL, &sc_proc_fops, bdev) == NULL)
				dev_err(&pdev->dev, "sc proc creation failed\n");

			if (rgn_proc_init(sdev->shared_mem) < 0)
				dprintk(VIP_ERR, "rgn proc init failed\n");
		}
	}

	return rc;
}

int sc_destroy_instance(struct platform_device *pdev)
{
	struct cvi_vip_dev *bdev;
	struct cvi_sc_vdev *sdev;

	bdev = dev_get_drvdata(&pdev->dev);
	if (!bdev) {
		dprintk(VIP_ERR, "invalid data\n");
		return -EINVAL;
	}

	remove_proc_entry(SC_PROC_NAME, NULL);
	vpss_proc_remove();
	rgn_proc_remove();
	sdev = &bdev->sc_vdev[0];
	kfree(sdev->shared_mem);

	return 0;
}

void sc_irq_handler(union sclr_intr intr_status, struct cvi_vip_dev *bdev)
{
	u8 i = 0;
	struct cvi_sc_vdev *sdev = NULL;

	for (i = 0; i < CVI_VIP_SC_MAX; ++i) {

		if ((i == 0 &&
			(intr_status.b.scl0_frame_end == 0 || bdev->sc_vdev[i].is_cmdq == true)) ||
		    (i == 1 && intr_status.b.scl1_frame_end == 0) ||
		    (i == 2 && intr_status.b.scl2_frame_end == 0) ||
		    (i == 3 && intr_status.b.scl3_frame_end == 0))
			continue;

		sdev = &bdev->sc_vdev[i];
		dprintk(VIP_DBG, "sc-%d: grp(%d) frame_end\n", i, sdev->job_grp);

		atomic_set(&sdev->job_state, CVI_VIP_IDLE);
		// in tile mode, only step forward if right-tile is done.
		if (bdev->img_vdev[sdev->img_src].is_tile) {
			if (!bdev->img_vdev[sdev->img_src].is_work_on_r_tile &&
			    (sdev->tile_mode != SCL_TILE_LEFT))
				goto sc_job_finish;

			sdev->tile_mode = SCL_TILE_MAX;
			sclr_set_scale_phase(i, 0, 0);
		}

		if ((i == CVI_VIP_SC_D) && bdev->disp_online)
			goto sc_job_finish;

		if (cvi_sc_buf_remove(sdev, sdev->job_grp))
			dprintk(VIP_ERR, "no sc%d buf, intr-status(%#x)\n", i, intr_status.raw);

sc_job_finish:
		cvi_vip_job_finish(&bdev->img_vdev[sdev->img_src]);
	}
}

void cvi_sc_update(struct cvi_sc_vdev *sdev, const struct cvi_vpss_chn_cfg *chn_cfg)
{
	const struct cvi_vip_fmt *fmt;
	struct sclr_odma_cfg *cfg;
	struct sclr_cir_cfg cir_cfg;
	struct sclr_size size;
	struct sclr_rect rect;

	dprintk(VIP_DBG, "-- sc%d --\n", sdev->dev_idx);
	dprintk(VIP_DBG, "%10s(%4d * %4d)%10s(%4d)\n", "src size", chn_cfg->src_size.width, chn_cfg->src_size.height,
		"sc coef", chn_cfg->sc_coef);
	dprintk(VIP_DBG, "%10s(%4d %4d %4d %4d)\n", "crop rect", chn_cfg->crop.left, chn_cfg->crop.top,
		chn_cfg->crop.width, chn_cfg->crop.height);
	dprintk(VIP_DBG, "%10s(%4d %4d %4d %4d)\n", "dst_rect", chn_cfg->dst_rect.left, chn_cfg->dst_rect.top,
		chn_cfg->dst_rect.width, chn_cfg->dst_rect.height);
	dprintk(VIP_DBG, "%10s(%4d)%10s(%4d)\n", "pitch_y", chn_cfg->bytesperline[0],
		"pitch_c", chn_cfg->bytesperline[1]);

	cfg = sclr_odma_get_cfg(sdev->dev_idx);

	// input
	size.w = chn_cfg->src_size.width;
	size.h = chn_cfg->src_size.height;
	sclr_set_input_size(sdev->dev_idx, size, true);
	vip_fill_rect_from_v4l2(rect, chn_cfg->crop);
	sclr_set_crop(sdev->dev_idx, rect, true);

	// fmt
	fmt = cvi_vip_get_format(chn_cfg->pixelformat);
	cfg->fmt = fmt->fmt;
	cfg->csc_cfg.work_on_border = true;
	if ((chn_cfg->pixelformat == V4L2_PIX_FMT_HSV24) || (chn_cfg->pixelformat == V4L2_PIX_FMT_HSVM))
		cfg->csc_cfg.mode = SCL_OUT_HSV;
	else if (chn_cfg->pixelformat == V4L2_PIX_FMT_YUV444M) {
		cfg->csc_cfg.mode = SCL_OUT_CSC;
		cfg->csc_cfg.csc_type = SCL_CSC_601_LIMIT_RGB2YUV;
	} else if (IS_YUV_FMT(cfg->fmt)) {
		cfg->csc_cfg.mode = SCL_OUT_CSC;
		cfg->csc_cfg.csc_type = (cfg->fmt == SCL_FMT_Y_ONLY) ? SCL_CSC_NONE : SCL_CSC_601_LIMIT_RGB2YUV;
	} else {
		cfg->csc_cfg.mode = SCL_OUT_DISABLE;
		cfg->csc_cfg.csc_type = SCL_CSC_NONE;
	}
	dprintk(VIP_DBG, "%10s(%4d)%10s(%4d)%10s(%4d)\n", "fmt", cfg->fmt, "csc mode", cfg->csc_cfg.mode,
		"csc_type", cfg->csc_cfg.csc_type);

	sclr_ctrl_set_output(sdev->dev_idx, &cfg->csc_cfg, cfg->fmt);
	if (cfg->csc_cfg.mode == SCL_OUT_CSC)
		sclr_set_csc(sdev->dev_idx, (struct sclr_csc_matrix *)&chn_cfg->csc_cfg);

	// update sc's output
	size.w = chn_cfg->dst_rect.width;
	size.h = chn_cfg->dst_rect.height;
	sclr_set_output_size(sdev->dev_idx, size);
	sclr_set_scale(sdev->dev_idx);

	if (chn_cfg->mute_cfg.enable) {
		cir_cfg.mode = SCL_CIR_SHAPE;
		cir_cfg.rect.x = 0;
		cir_cfg.rect.y = 0;
		cir_cfg.rect.w = chn_cfg->dst_size.width;
		cir_cfg.rect.h = chn_cfg->dst_size.height;
		cir_cfg.center.x = chn_cfg->dst_size.width >> 1;
		cir_cfg.center.y = chn_cfg->dst_size.height >> 1;
		cir_cfg.radius = MAX(cir_cfg.rect.w, cir_cfg.rect.h);
		cir_cfg.color_r = chn_cfg->mute_cfg.color[0];
		cir_cfg.color_g = chn_cfg->mute_cfg.color[1];
		cir_cfg.color_b = chn_cfg->mute_cfg.color[2];
	} else
		cir_cfg.mode = SCL_CIR_DISABLE;
	sclr_cir_set_cfg(sdev->dev_idx, &cir_cfg);

	// update out-2-mem's pos & size
	cfg->mem.pitch_y = chn_cfg->bytesperline[0];
	cfg->mem.pitch_c = chn_cfg->bytesperline[1];
	cfg->flip = chn_cfg->flip;
	cfg->frame_size.w = chn_cfg->dst_size.width;
	cfg->frame_size.h = chn_cfg->dst_size.height;
	cfg->mem.width	 = chn_cfg->dst_rect.width;
	cfg->mem.height  = chn_cfg->dst_rect.height;
	if ((cfg->flip == SCL_FLIP_HFLIP) || (cfg->flip == SCL_FLIP_HVFLIP))
		cfg->mem.start_x = cfg->frame_size.w - chn_cfg->dst_rect.width - chn_cfg->dst_rect.left;
	else
		cfg->mem.start_x = chn_cfg->dst_rect.left;
	if ((cfg->flip == SCL_FLIP_VFLIP) || (cfg->flip == SCL_FLIP_HVFLIP))
		cfg->mem.start_y = cfg->frame_size.h - chn_cfg->dst_rect.height - chn_cfg->dst_rect.top;
	else
		cfg->mem.start_y = chn_cfg->dst_rect.top;
	sclr_odma_set_cfg(sdev->dev_idx, cfg);

	_sc_ext_set_border(sdev, &chn_cfg->border_cfg);
	dprintk(VIP_DBG, "%10s(%4d)%10s(%4d %4d)%10s(%4d %4d %4d)\n",
		"border enable", chn_cfg->border_cfg.enable,
		"offset", chn_cfg->border_cfg.offset_x, chn_cfg->border_cfg.offset_y,
		"bgcolor", chn_cfg->border_cfg.bg_color[0], chn_cfg->border_cfg.bg_color[1],
		chn_cfg->border_cfg.bg_color[2]);
	_sc_ext_set_quant(sdev, &chn_cfg->quant_cfg);
	dprintk(VIP_DBG, "%10s(%4d)%10s(%4d)%10s(%4d %4d %4d)%10s(%4d %4d %4d)%10s(%4d %4d %4d)\n",
		"quant enable", chn_cfg->quant_cfg.enable,
		"rounding", chn_cfg->quant_cfg.rounding,
		"factor", chn_cfg->quant_cfg.sc_frac[0], chn_cfg->quant_cfg.sc_frac[1], chn_cfg->quant_cfg.sc_frac[2],
		"sub", chn_cfg->quant_cfg.sub[0], chn_cfg->quant_cfg.sub[1], chn_cfg->quant_cfg.sub[2],
		"sub_frac", chn_cfg->quant_cfg.sub_frac[0], chn_cfg->quant_cfg.sub_frac[1],
		chn_cfg->quant_cfg.sub_frac[2]);

	if (sdev->bind_fb) {
		struct sclr_gop_cfg *disp_cfg = sclr_gop_get_cfg(SCL_GOP_DISP);
		struct sclr_gop_cfg gop_cfg = *disp_cfg;

		sclr_gop_ow_set_cfg(sdev->dev_idx, 0, &disp_cfg->ow_cfg[0], true);
		gop_cfg.gop_ctrl.b.ow0_en = true;
		sclr_gop_set_cfg(sdev->dev_idx, &gop_cfg, true);
		dprintk(VIP_DBG, "use fb bind\n");
	} else
		cvi_vip_set_rgn_cfg(sdev->dev_idx, &chn_cfg->rgn_cfg, &sclr_get_cfg(sdev->dev_idx)->sc.dst);

	if (chn_cfg->sc_coef <= CVI_SC_SCALING_COEF_Z3) {
		sclr_update_coef(sdev->dev_idx, chn_cfg->sc_coef, NULL);
	} else if (chn_cfg->sc_coef == CVI_SC_SCALING_COEF_OPENCV_BILINEAR) {
		sclr_set_opencv_scale(sdev->dev_idx);
		sclr_update_coef(sdev->dev_idx, CVI_SC_SCALING_COEF_BILINEAR, NULL);
	}
}
