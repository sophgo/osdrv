/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_vip_img.c
 * Description: scaler input driver
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/videodev2.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-event.h>

#include "vip/vip_common.h"
#include "vip/scaler.h"

#include "cvi_debug.h"
#include "cvi_vip_core.h"
#include "cvi_vip_img.h"
#include "cvi_vip_sc.h"

static const char *const CLK_IMG_NAME[] = {"clk_img_d", "clk_img_v"};
static struct sclr_rgnex_list g_sclr_rgnex_list;
static void *sclr_cmdq_addr;

/*************************************************************************
 *	Local function
 *************************************************************************/
static int _img_map_rgnex_gop_cfg(const struct cvi_rgn_ex_cfg *cfg,
	struct sclr_gop_cfg *gop_cfg, int index)
{
	struct sclr_gop_ow_cfg *ow_cfg;
	u8 bpp;
	struct v4l2_rect rgnex_rect;

	gop_cfg->gop_ctrl.raw &= ~0xfff;
	gop_cfg->gop_ctrl.b.hscl_en = cfg->hscale_x2;
	gop_cfg->gop_ctrl.b.vscl_en = cfg->vscale_x2;
	gop_cfg->gop_ctrl.b.colorkey_en = cfg->colorkey_en;
	gop_cfg->colorkey = cfg->colorkey;

	bpp = (cfg->rgn_ex_param[index].fmt == CVI_RGN_FMT_ARGB8888) ? 4 :
	(cfg->rgn_ex_param[index].fmt == CVI_RGN_FMT_256LUT) ? 1 : 2;
	rgnex_rect = cfg->rgn_ex_param[index].rect;
	rgnex_rect.left = rgnex_rect.top = 0;

	ow_cfg = &gop_cfg->ow_cfg[0];
	gop_cfg->gop_ctrl.raw |= BIT(0);

	ow_cfg->fmt = cfg->rgn_ex_param[index].fmt;
	ow_cfg->addr = cfg->rgn_ex_param[index].phy_addr;
	ow_cfg->pitch = cfg->rgn_ex_param[index].stride;
	ow_cfg->start.x = rgnex_rect.left;
	ow_cfg->img_size.w = rgnex_rect.width;
	ow_cfg->start.y = rgnex_rect.top;
	ow_cfg->img_size.h = rgnex_rect.height;
	ow_cfg->end.x = ow_cfg->start.x +
			(ow_cfg->img_size.w << gop_cfg->gop_ctrl.b.hscl_en) - gop_cfg->gop_ctrl.b.hscl_en;
	ow_cfg->end.y = ow_cfg->start.y +
			(ow_cfg->img_size.h << gop_cfg->gop_ctrl.b.vscl_en) - gop_cfg->gop_ctrl.b.vscl_en;
	ow_cfg->mem_size.w = ALIGN(ow_cfg->img_size.w * bpp, GOP_ALIGNMENT);
	ow_cfg->mem_size.h = ow_cfg->img_size.h;

	dprintk(VIP_INFO, "gop fmt(%d) rect(%d %d %d %d) addr(%llx) pitch(%d).\n"
		, ow_cfg->fmt, ow_cfg->start.x, ow_cfg->start.y, ow_cfg->img_size.w, ow_cfg->img_size.h
		, ow_cfg->addr, ow_cfg->pitch);

	return 0;
}

static int _img_s_rgnex_cfg(struct cvi_vpss_rgnex_cfg *vpss_rgnex_cfg)
{
	const struct cvi_vip_fmt *fmt;
	struct sclr_rgnex_cfg *cfg;
	int i;

	memset(&g_sclr_rgnex_list, 0, sizeof(g_sclr_rgnex_list));
	g_sclr_rgnex_list.num_of_cfg = vpss_rgnex_cfg->cfg.num_of_rgn_ex;
	for (i = 0; i < g_sclr_rgnex_list.num_of_cfg; ++i) {
		cfg = &g_sclr_rgnex_list.cfg[i];

		fmt = cvi_vip_get_format(vpss_rgnex_cfg->pixelformat);
		cfg->fmt = fmt->fmt;
		if (vpss_rgnex_cfg->pixelformat == V4L2_PIX_FMT_YUV444M) {
			cfg->src_csc = SCL_CSC_601_LIMIT_YUV2RGB;
			cfg->dst_csc = SCL_CSC_601_LIMIT_RGB2YUV;
		} else if (IS_YUV_FMT(cfg->fmt)) {
			cfg->src_csc = (cfg->fmt == SCL_FMT_Y_ONLY) ? SCL_CSC_NONE : SCL_CSC_601_LIMIT_YUV2RGB;
			cfg->dst_csc = (cfg->fmt == SCL_FMT_Y_ONLY) ? SCL_CSC_NONE : SCL_CSC_601_LIMIT_RGB2YUV;
		} else
			cfg->src_csc = cfg->dst_csc = SCL_CSC_NONE;

		cfg->bytesperline[0] = vpss_rgnex_cfg->bytesperline[0];
		cfg->bytesperline[1] = vpss_rgnex_cfg->bytesperline[1];
		cfg->addr0 = vpss_rgnex_cfg->addr[0];
		cfg->addr1 = vpss_rgnex_cfg->addr[1];
		cfg->addr2 = vpss_rgnex_cfg->addr[2];
		cfg->rgnex_rect.x = vpss_rgnex_cfg->cfg.rgn_ex_param[i].rect.left;
		cfg->rgnex_rect.y = vpss_rgnex_cfg->cfg.rgn_ex_param[i].rect.top;
		cfg->rgnex_rect.w = vpss_rgnex_cfg->cfg.rgn_ex_param[i].rect.width;
		cfg->rgnex_rect.h = vpss_rgnex_cfg->cfg.rgn_ex_param[i].rect.height;
		_img_map_rgnex_gop_cfg(&vpss_rgnex_cfg->cfg, &cfg->gop_cfg, i);
	}

	dprintk(VIP_INFO, "num_of_cfg(%d).\n", g_sclr_rgnex_list.num_of_cfg);
	for (i = 0; i < g_sclr_rgnex_list.num_of_cfg; ++i) {
		dprintk(VIP_INFO, "fmt(%d), src_csc(%d), dst_csc(%d), addr(0x%llx, 0x%llx, 0x%llx)\n",
			g_sclr_rgnex_list.cfg[i].fmt,
			g_sclr_rgnex_list.cfg[i].src_csc,
			g_sclr_rgnex_list.cfg[i].dst_csc,
			g_sclr_rgnex_list.cfg[i].addr0,
			g_sclr_rgnex_list.cfg[i].addr1,
			g_sclr_rgnex_list.cfg[i].addr2);

		dprintk(VIP_INFO, "rect_rgnex(%d, %d, %d, %d), bytesperline(%d, %d)\n",
			g_sclr_rgnex_list.cfg[i].rgnex_rect.x,
			g_sclr_rgnex_list.cfg[i].rgnex_rect.y,
			g_sclr_rgnex_list.cfg[i].rgnex_rect.w,
			g_sclr_rgnex_list.cfg[i].rgnex_rect.h,
			g_sclr_rgnex_list.cfg[i].bytesperline[0],
			g_sclr_rgnex_list.cfg[i].bytesperline[1]);
	}

	return 0;
}

/*************************************************************************
 *	VB2_OPS definition
 *************************************************************************/
/**
 * call before VIDIOC_REQBUFS to setup buf-queue.
 * nbuffers: number of buffer requested
 * nplanes:  number of plane each buffer
 * sizes:    size of each plane(bytes)
 */
static int cvi_img_queue_setup(struct vb2_queue *vq,
	       unsigned int *nbuffers, unsigned int *nplanes,
	       unsigned int sizes[], struct device *alloc_devs[])
{
	struct cvi_img_vdev *idev = vb2_get_drv_priv(vq);
	unsigned int planes = idev->fmt->buffers;
	unsigned int p;

	dprintk(VIP_VB2, "(%d)+\n", idev->dev_idx);

	// just give the min_length
	for (p = 0; p < planes; ++p)
		sizes[p] = idev->sizeimage[p];

	if (vq->num_buffers + *nbuffers < 2)
		*nbuffers = 2 - vq->num_buffers;

	*nplanes = idev->fmt->buffers;

	dprintk(VIP_INFO, "num_buffer=%d, num_plane=%d\n", *nbuffers, *nplanes);
	for (p = 0; p < *nplanes; p++)
		dprintk(VIP_INFO, "size[%u]=%u\n", p, sizes[p]);

	return 0;
}

/**
 * for VIDIOC_STREAMON, start fill data.
 */
static void cvi_img_buf_queue(struct vb2_buffer *vb)
{
	struct cvi_img_vdev *idev = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct cvi_vip_buffer *cvi_vb2 =
		container_of(vbuf, struct cvi_vip_buffer, vb);

	dprintk(VIP_VB2, "(%d)+\n", idev->dev_idx);

	cvi_vip_buf_queue((struct cvi_base_vdev *)idev, cvi_vb2);

	//cvi_vip_try_schedule(idev, 0);
}

static int cvi_img_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct cvi_img_vdev *idev = vb2_get_drv_priv(vq);
	int rc = 0;

	dprintk(VIP_VB2, "(%d)+\n", idev->dev_idx);

	idev->seq_count = 0;

	return rc;
}

/* abort streaming and wait for last buffer */
static void cvi_img_stop_streaming(struct vb2_queue *vq)
{
	struct cvi_img_vdev *idev = vb2_get_drv_priv(vq);
	struct cvi_vip_buffer *cvi_vb2, *tmp;
	unsigned long flags;
	struct vb2_buffer *vb2_buf;

	dprintk(VIP_VB2, "(%d)+\n", idev->dev_idx);

	/*
	 * Release all the buffers enqueued to driver
	 * when streamoff is issued
	 */
	spin_lock_irqsave(&idev->rdy_lock, flags);
	list_for_each_entry_safe(cvi_vb2, tmp, &(idev->rdy_queue), list) {
		vb2_buf = &(cvi_vb2->vb.vb2_buf);
		if (vb2_buf->state == VB2_BUF_STATE_DONE)
			continue;
		vb2_buffer_done(vb2_buf, VB2_BUF_STATE_DONE);
	}
	idev->num_rdy = 0;
	INIT_LIST_HEAD(&idev->rdy_queue);
	spin_unlock_irqrestore(&idev->rdy_lock, flags);
}

const struct vb2_ops cvi_img_qops = {
//    .buf_init           =
	.queue_setup        = cvi_img_queue_setup,
//    .buf_finish         = cvi_img_buf_finish,
	.buf_queue          = cvi_img_buf_queue,
	.start_streaming    = cvi_img_start_streaming,
	.stop_streaming     = cvi_img_stop_streaming,
//    .wait_prepare       = vb2_ops_wait_prepare,
//    .wait_finish        = vb2_ops_wait_finish,
};

/*************************************************************************
 *	VB2-MEM-OPS definition
 *************************************************************************/
static void *img_get_userptr(struct device *dev, unsigned long vaddr,
	unsigned long size, enum dma_data_direction dma_dir)
{
	return (void *)0xdeadbeef;
}

static void img_put_userptr(void *buf_priv)
{
}

static const struct vb2_mem_ops cvi_img_vb2_mem_ops = {
	.get_userptr = img_get_userptr,
	.put_userptr = img_put_userptr,
};

/*************************************************************************
 *	FOPS definition
 *************************************************************************/
static int cvi_img_open(struct file *file)
{
	int rc = 0;
	struct cvi_img_vdev *idev = video_drvdata(file);
	struct cvi_vip_dev *bdev = container_of(idev, struct cvi_vip_dev, img_vdev[idev->dev_idx]);

	WARN_ON(!idev);

	rc = v4l2_fh_open(file);
	if (rc) {
		dprintk(VIP_ERR, "v4l2_fh_open failed(%d)\n", rc);
		return rc;
	}

	if (v4l2_fh_is_singular_file(file)) {
		if (bdev->clk_sys[1])
			clk_prepare_enable(bdev->clk_sys[1]);
		if (bdev->clk_sc_top)
			clk_prepare_enable(bdev->clk_sc_top);
		if ((debug & BIT(2)) && idev->clk)
			clk_prepare_enable(idev->clk);

		sclr_img_reg_shadow_sel(idev->img_type, false);
	}

	idev->irq_cnt = idev->ol_irq_cnt[0] = idev->ol_irq_cnt[1] = 0;
	idev->isp_trig_cnt[0] = idev->isp_trig_cnt[1] = 0;
	idev->isp_trig_fail_cnt[0] = idev->isp_trig_fail_cnt[1] = 0;
	idev->user_trig_cnt = 0;
	idev->user_trig_fail_cnt = 0;
	idev->frame_number = 0;

	dprintk(VIP_INFO, "by %s\n", current->comm);
	return rc;
}

static int cvi_img_release(struct file *file)
{
	struct cvi_img_vdev *idev = video_drvdata(file);

	WARN_ON(!idev);

	if (vb2_is_streaming(&idev->vb_q))
		vb2_streamoff(&idev->vb_q, idev->vb_q.type);

	if (v4l2_fh_is_singular_file(file)) {
		struct cvi_vip_dev *bdev = container_of(idev, struct cvi_vip_dev, img_vdev[idev->dev_idx]);

		if (!(debug & BIT(2)) && idev->clk && __clk_is_enabled(idev->clk))
			clk_disable_unprepare(idev->clk);
		if (!(debug & BIT(2)) && bdev->clk_sys[1] && __clk_is_enabled(bdev->clk_sys[1]))
			clk_disable_unprepare(bdev->clk_sys[1]);
		if (!(debug & BIT(2)) && bdev->clk_sc_top && __clk_is_enabled(bdev->clk_sc_top))
			clk_disable_unprepare(bdev->clk_sc_top);
	}

	vb2_fop_release(file);

	dprintk(VIP_INFO, "-\n");
	return 0;
}

#if 0
static unsigned int cvi_img_poll(struct file *file,
	struct poll_table_struct *wait)
{
	struct cvi_img_vdev *idev = video_drvdata(file);
	struct vb2_buffer *cap_vb = NULL;
	unsigned long flags;
	int rc = 0;

	WARN_ON(!idev);

	poll_wait(file, &idev->vb_q.done_wq, wait);
	rc = vb2_fop_poll(file, wait);
	spin_lock_irqsave(&idev->rdy_lock, flags);
	if (!list_empty(&idev->vb_q.done_list))
	cap_vb = list_first_entry(&idev->vb_q.done_list, struct vb2_buffer,
				  done_entry);
	if (cap_vb && (cap_vb->state == VB2_BUF_STATE_DONE
		|| cap_vb->state == VB2_BUF_STATE_ERROR))
	rc |= POLLIN | POLLRDNORM;
	spin_unlock_irqrestore(&idev->rdy_lock, flags);
	return rc;
}
#endif

static struct v4l2_file_operations cvi_img_fops = {
	.owner = THIS_MODULE,
	.open = cvi_img_open,
	.release = cvi_img_release,
	.poll = vb2_fop_poll, //    .poll = cvi_img_poll,
	.unlocked_ioctl = video_ioctl2,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = video_ioctl2,
#endif
};

/*************************************************************************
 *	IOCTL definition
 *************************************************************************/
static int cvi_img_querycap(struct file *file, void *priv,
		    struct v4l2_capability *cap)
{
	struct cvi_img_vdev *idev = video_drvdata(file);
	struct cvi_vip_dev *bdev =
		container_of(idev, struct cvi_vip_dev, img_vdev[idev->dev_idx]);

	strlcpy(cap->driver, CVI_VIP_DRV_NAME, sizeof(cap->driver));
	strlcpy(cap->card, CVI_VIP_DVC_NAME, sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info),
	    "platform:%s", bdev->v4l2_dev.name);

	cap->capabilities = idev->vid_caps | V4L2_CAP_DEVICE_CAPS;
	return 0;
}

static int cvi_img_g_ctrl(struct file *file, void *priv, struct v4l2_control *vc)
{
	int rc = -EINVAL;
	return rc;
}

static int cvi_img_s_ctrl(struct file *file, void *priv, struct v4l2_control *vc)
{
	int rc = -EINVAL;
	return rc;
}

static int cvi_img_s_ext_ctrls(struct file *file, void *priv,
	struct v4l2_ext_controls *vc)
{
	struct v4l2_ext_control *ext_ctrls;
	int rc = -EINVAL, i = 0;
	struct cvi_img_vdev *idev = video_drvdata(file);
	union sclr_intr intr_mask;

	ext_ctrls = vc->controls;
	for (i = 0; i < vc->count; ++i) {
		switch (ext_ctrls[i].id) {
		case V4L2_CID_DV_VIP_IMG_INTR: {
			static bool service_isr = true;

			service_isr = !service_isr;
			dprintk(VIP_DBG, "service_irs(%d)\n", service_isr);
			intr_mask = sclr_get_intr_mask();
			if (idev->img_type == SCL_IMG_D)
				intr_mask.b.img_in_d_frame_end = (service_isr) ? 1 : 0;
			else
				intr_mask.b.img_in_v_frame_end = (service_isr) ? 1 : 0;
			sclr_set_intr_mask(intr_mask);
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_IMG_SET_ALIGN: {
			if (ext_ctrls[i].value >= VIP_ALIGNMENT) {
				idev->align = ext_ctrls[i].value;
				rc = 0;
			}
		}
		break;

		case V4L2_CID_DV_VIP_IMG_SET_VPSS_GRP_CFG: {
			struct cvi_vpss_grp_cfg cfg;

			if (copy_from_user(&cfg, ext_ctrls[i].ptr, sizeof(cfg))) {
				dprintk(VIP_ERR, "ioctl-%#x, copy_from_user failed.\n", ext_ctrls[i].id);
				rc = -ENOMEM;
				break;
			}

			if (cfg.is_online)
				idev->ol_vpss_grp_cfg[cfg.grp_id] = cfg;
			else
				idev->vpss_grp_cfg = cfg;
			dprintk(VIP_INFO, "grp(%d) chn_en(%d %d %d %d).\n", cfg.grp_id, cfg.chn_enable[0],
				cfg.chn_enable[1], cfg.chn_enable[2], cfg.chn_enable[3]);
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_IMG_CLR_VPSS_GRP_CFG: {
			memset(idev->ol_vpss_grp_cfg, 0, sizeof(idev->ol_vpss_grp_cfg));
			memset(&idev->vpss_grp_cfg, 0, sizeof(struct cvi_vpss_grp_cfg));
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_IMG_TRIGGER_RGNEX: {
			struct cvi_vip_dev *bdev =
				container_of(idev, struct cvi_vip_dev, img_vdev[idev->dev_idx]);
			struct cvi_sc_vdev *sdev;
			struct sclr_top_cfg *top_cfg;

			idev = &bdev->img_vdev[0]; // img_d
			sdev = &bdev->sc_vdev[0]; // sc_d

			if (copy_from_user(&idev->rgnex_cfg, ext_ctrls[i].ptr,
					sizeof(struct cvi_vpss_rgnex_cfg))) {
				dprintk(VIP_ERR, "ioctl-%#x, copy_from_user failed.\n", ext_ctrls[i].id);
				rc = -ENOMEM;
				break;
			}
			_img_s_rgnex_cfg(&idev->rgnex_cfg);

			if (!(debug & BIT(2)) && idev->clk)
				clk_prepare_enable(idev->clk);
			if (!(debug & BIT(2)) && sdev->clk)
				clk_prepare_enable(sdev->clk);
			idev->is_cmdq = true;
			sdev->is_cmdq = true;

			top_cfg = sclr_top_get_cfg();
			top_cfg->sclr_enable[0] = true;
			top_cfg->sclr_d_src = 0;
			sclr_top_set_cfg(top_cfg);
			sclr_engine_cmdq_rgnex(g_sclr_rgnex_list.cfg,
				g_sclr_rgnex_list.num_of_cfg, (uintptr_t)sclr_cmdq_addr);
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_IMG_RGNEX_DONE: {
			struct cvi_vip_dev *bdev =
				container_of(idev, struct cvi_vip_dev, img_vdev[idev->dev_idx]);
			struct cvi_sc_vdev *sdev;
			struct sclr_top_cfg *top_cfg;

			idev = &bdev->img_vdev[0]; // img_d
			sdev = &bdev->sc_vdev[0]; // sc_d

			top_cfg = sclr_top_get_cfg();
			top_cfg->sclr_enable[0] = false;
			sclr_top_set_cfg(top_cfg);

			if (!(debug & BIT(2)) && idev->clk)
				clk_disable_unprepare(idev->clk);
			if (!(debug & BIT(2)) && sdev->clk)
				clk_disable_unprepare(sdev->clk);

			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_IMG_RGNEX_RESET: {
			struct cvi_vip_dev *bdev =
				container_of(idev, struct cvi_vip_dev, img_vdev[idev->dev_idx]);
			union sclr_img_dbg_status status;

			idev = &bdev->img_vdev[0]; // img_d
			status = sclr_img_get_dbg_status(idev->img_type, true);

			dprintk(VIP_INFO, "img(%d) isn't idle\n", idev->dev_idx);
			dprintk(VIP_INFO, "err_fwr_yuv(%d%d%d)\terr_erd_yuv(%d%d%d)\n"
				  , status.b.err_fwr_y, status.b.err_fwr_u,  status.b.err_fwr_v
				  , status.b.err_erd_y, status.b.err_erd_u, status.b.err_erd_v);
			dprintk(VIP_INFO, "tlb_full_yuv(%d%d%d)\tlb_empty_yuv(%d%d%d)\n"
				  , status.b.lb_full_y, status.b.lb_full_u, status.b.lb_full_v
				  , status.b.lb_empty_y, status.b.lb_empty_u, status.b.lb_empty_v);
			dprintk(VIP_INFO, "ip idle(%d)\t\tip int(%d)\n", status.b.ip_idle, status.b.ip_int);
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_IMG_OFFLINE_TRIG: {
			idev->user_trig_cnt++;

			rc = cvi_vip_try_schedule(idev, 0, false);
			if (rc) {
				dprintk(VIP_ERR, "img(%d) offline trig fail\n", idev->dev_idx);
				idev->user_trig_fail_cnt++;
				break;
			}
			rc = 0;
		}
		break;

		default:
		break;
		}
	}
	return rc;
}

int cvi_img_g_selection(struct file *file, void *priv,
		struct v4l2_selection *sel)
{
	struct cvi_img_vdev *idev = video_drvdata(file);
	int rc = -EINVAL;

	dprintk(VIP_DBG, "(%d)+\n", idev->dev_idx);

	sel->r.left = sel->r.top = 0;
	switch (sel->target) {
	case V4L2_SEL_TGT_CROP_BOUNDS:
	case V4L2_SEL_TGT_CROP_DEFAULT:
		sel->r.top = sel->r.left = 0;
		sel->r.width = idev->src_size.width;
		sel->r.height = idev->src_size.height;
		rc = 0;
	break;

	case V4L2_SEL_TGT_CROP:
		sel->r = idev->crop_rect;
		rc = 0;
	break;

	default:
		return rc;
	}

	dprintk(VIP_INFO, "target(%d) rect(%d %d %d %d)\n", sel->target,
			sel->r.left, sel->r.top, sel->r.width, sel->r.height);
	return rc;
}

#if 0
int cvi_img_s_selection(struct file *file, void *fh, struct v4l2_selection *sel)
{
	struct cvi_img_vdev *idev = video_drvdata(file);
	int rc = -EINVAL;

	dprintk(VIP_DBG, "(%d)+\n", idev->dev_idx);

	if (cvi_vip_job_is_queued(idev)) {
		dprintk(VIP_ERR, "job in queue\n");
		rc = -EAGAIN;
		return rc;
	}

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP:
		if (memcmp(&idev->crop_rect, &sel->r, sizeof(sel->r))) {
			idev->vpss_grp_cfg[0].crop = sel->r;
			idev->crop_rect = sel->r;
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
#endif

int cvi_img_enum_fmt_vid_mplane(struct file *file, void  *priv,
		    struct v4l2_fmtdesc *f)
{
	dprintk(VIP_DBG, "+\n");
	return cvi_vip_enum_fmt_vid(file, priv, f);
}

int cvi_img_g_fmt_vid_cap_mplane(struct file *file, void *priv,
		    struct v4l2_format *f)
{
	struct cvi_img_vdev *idev = video_drvdata(file);
	struct v4l2_pix_format_mplane *mp = &f->fmt.pix_mp;
	unsigned int p;

	dprintk(VIP_DBG, "(%d)+\n", idev->dev_idx);
	WARN_ON(!idev);

	mp->width        = idev->crop_rect.width;
	mp->height       = idev->crop_rect.height;
	mp->field        = V4L2_FIELD_NONE;
	mp->pixelformat  = idev->fmt->fourcc;
	mp->colorspace   = idev->colorspace;
	mp->xfer_func    = V4L2_XFER_FUNC_DEFAULT;
	mp->ycbcr_enc    = V4L2_YCBCR_ENC_DEFAULT;
	mp->quantization = V4L2_QUANTIZATION_DEFAULT;
	mp->num_planes   = idev->fmt->buffers;
	for (p = 0; p < mp->num_planes; p++) {
		mp->plane_fmt[p].bytesperline = idev->bytesperline[p];
		mp->plane_fmt[p].sizeimage = idev->sizeimage[p];
	}

	return 0;
}

int cvi_img_try_fmt_vid_cap_mplane(struct file *file, void *priv,
	    struct v4l2_format *f)
{
	struct cvi_img_vdev *idev = video_drvdata(file);

	return cvi_vip_try_fmt_vid_mplane(f, idev->align);
}

int cvi_img_s_fmt_vid_cap_mplane(struct file *file, void *priv,
	    struct v4l2_format *f)
{
	struct cvi_img_vdev *idev = video_drvdata(file);
	struct v4l2_pix_format_mplane *mp = &f->fmt.pix_mp;
	struct v4l2_plane_pix_format *pfmt = mp->plane_fmt;
	const struct cvi_vip_fmt *fmt;
	unsigned int p;
	int rc = cvi_img_try_fmt_vid_cap_mplane(file, priv, f);

	dprintk(VIP_DBG, "(%d)+\n", idev->dev_idx);
	if (rc < 0)
		return rc;

	if (cvi_vip_job_is_queued(idev)) {
		dprintk(VIP_ERR, "job in queue\n");
		return -EINVAL;
	}

	fmt = cvi_vip_get_format(mp->pixelformat);
	idev->fmt = fmt;
	idev->colorspace = mp->colorspace;
	for (p = 0; p < mp->num_planes; p++) {
		idev->bytesperline[p] = pfmt[p].bytesperline;
		idev->sizeimage[p] = pfmt[p].sizeimage;
	}
	idev->src_size.width = mp->width;
	idev->src_size.height = mp->height;
	idev->crop_rect.left = idev->crop_rect.top = 0;
	idev->crop_rect.width = mp->width;
	idev->crop_rect.height = mp->height;
	dprintk(VIP_INFO, "src size(%d-%d) crop size(%d-%d)\n",
			idev->src_size.width, idev->src_size.height,
			idev->crop_rect.width, idev->crop_rect.height);
	return rc;
}

int cvi_img_streamon(struct file *file, void *fh, enum v4l2_buf_type i)
{
	struct cvi_img_vdev *idev = video_drvdata(file);
	struct cvi_vip_dev *bdev =
		container_of(idev, struct cvi_vip_dev, img_vdev[idev->dev_idx]);
	int rc = 0;

	if (atomic_cmpxchg(&idev->is_streaming, 0, 1) != 0) {
		dprintk(VIP_DBG, "img(%d) is running\n", idev->dev_idx);
		return rc;
	}

	if ((idev->img_type == SCL_IMG_D) && (idev->input_type != CVI_VIP_INPUT_MEM) && bdev->disp_online) {
		dprintk(VIP_ERR, "IMG_D can't enable both disp_online and input from %s.\n"
			, (idev->input_type ? "ISP" : "DWA"));
		return -EPERM;
	}

	rc = vb2_streamon(&idev->vb_q, i);
	if (!rc) {
		if (!(debug & BIT(2)) && idev->clk)
			clk_prepare(idev->clk);
		tasklet_enable(&idev->job_work);
	}

	return rc;
}

int cvi_img_streamoff(struct file *file, void *fh, enum v4l2_buf_type i)
{
	struct cvi_img_vdev *idev = video_drvdata(file);
	int rc = 0, count = 10;

	if (atomic_cmpxchg(&idev->is_streaming, 1, 0) != 1) {
		dprintk(VIP_DBG, "img(%d) is off\n", idev->dev_idx);
		return rc;
	}

	tasklet_disable(&idev->job_work);

	while (--count > 0) {
		if (!cvi_vip_job_is_queued(idev))
			break;
		dprintk(VIP_DBG, "wait count(%d)\n", count);
		usleep_range(5 * 1000, 10 * 1000);
	}

	if (count == 0) {
		union sclr_img_dbg_status status = sclr_img_get_dbg_status(idev->img_type, true);

		dprintk(VIP_ERR, "img(%d) isn't idle\n", idev->dev_idx);
		dprintk(VIP_ERR, "err_fwr_yuv(%d%d%d)\terr_erd_yuv(%d%d%d)\tlb_full_yuv(%d%d%d)\tlb_empty_yuv(%d%d%d)\n"
			  , status.b.err_fwr_y, status.b.err_fwr_u,  status.b.err_fwr_v, status.b.err_erd_y
			  , status.b.err_erd_u, status.b.err_erd_v, status.b.lb_full_y, status.b.lb_full_u
			  , status.b.lb_full_v, status.b.lb_empty_y, status.b.lb_empty_u, status.b.lb_empty_v);
		dprintk(VIP_ERR, "ip idle(%d)\t\tip int(%d)\n", status.b.ip_idle, status.b.ip_int);

		sclr_img_reset(idev->dev_idx);
	}

	if (!(debug & BIT(2)) && idev->clk)
		while (__clk_is_enabled(idev->clk))
			clk_disable(idev->clk);

	rc = vb2_streamoff(&idev->vb_q, i);
	if (rc)
		return rc;

	if (!(debug & BIT(2)) && idev->clk)
		clk_unprepare(idev->clk);

	idev->job_flags = 0;
	idev->is_tile = false;
	idev->is_work_on_r_tile = true;
	idev->is_online_from_isp = false;
	return rc;
}

int cvi_img_enum_input(struct file *file, void *priv, struct v4l2_input *inp)
{
	int rc = 0;
	return rc;
}

int cvi_img_g_input(struct file *file, void *priv, unsigned int *i)
{
	struct cvi_img_vdev *idev = video_drvdata(file);
	int rc = 0;
	*i = idev->input_type;
	return rc;
}

int cvi_img_get_input(enum sclr_img_in img_type,
	enum cvi_input_type input_type, enum sclr_input *input)
{
	if (img_type == SCL_IMG_D) {
		if (input_type == CVI_VIP_INPUT_DWA) {
			dprintk(VIP_ERR, "img_d doesn't have dwa input.\n");
			return -EINVAL;
		}

		*input = (input_type == CVI_VIP_INPUT_ISP || input_type == CVI_VIP_INPUT_ISP_POST) ?
			SCL_INPUT_ISP : SCL_INPUT_MEM;
	} else {
		*input = (input_type == CVI_VIP_INPUT_ISP || input_type == CVI_VIP_INPUT_ISP_POST) ?
			SCL_INPUT_ISP : (input_type == CVI_VIP_INPUT_DWA) ?
			SCL_INPUT_DWA : SCL_INPUT_MEM;
	}
	return 0;
}


int cvi_img_s_input(struct file *file, void *priv, unsigned int i)
{
	struct cvi_img_vdev *idev = video_drvdata(file);
	int rc = 0;
	enum sclr_input input;
	struct sclr_img_cfg *cfg;
	unsigned long flags_job;

	dprintk(VIP_DBG, "(%d) input(%d)+\n", idev->dev_idx, i);
	if (i >= CVI_VIP_INPUT_MAX)
		return -EINVAL;

	spin_lock_irqsave(&idev->job_lock, flags_job);
	if (cvi_vip_job_is_queued(idev) || idev->is_tile) {
		dprintk(VIP_ERR, "job in queue\n");
		spin_unlock_irqrestore(&idev->job_lock, flags_job);
		return -EAGAIN;
	}

	idev->input_type = i;

	// update hw
	cfg = sclr_img_get_cfg(idev->img_type);
	rc = cvi_img_get_input(idev->img_type, i, &input);
	if (rc == 0) {
		idev->is_online_from_isp = (i == CVI_VIP_INPUT_ISP) || (i == CVI_VIP_INPUT_ISP_POST);
		sclr_ctrl_set_input(idev->img_type, input, cfg->fmt, cfg->csc, i == CVI_VIP_INPUT_ISP);
	}
	spin_unlock_irqrestore(&idev->job_lock, flags_job);
	return rc;
}

void cvi_img_v4l2_event_queue(
	struct cvi_img_vdev *vdev, const u32 type, const u8 grp_id, const u8 sc_early_ids)
{
	struct v4l2_event event = {
		.type = type,
		.u.data[0] = grp_id,
	};
	u32 diff;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct timespec64 ts;
	uint64_t dividend;

	ktime_get_ts64(&ts);
	event.timestamp.tv_sec = ts.tv_sec;
	event.timestamp.tv_nsec = ts.tv_nsec;

	dividend = event.timestamp.tv_nsec - vdev->ts_start.tv_nsec;
	do_div(dividend, 1000);
	diff = (uint32_t)dividend;
	diff += (event.timestamp.tv_sec - vdev->ts_start.tv_sec) * 1000000;
#else
	ktime_get_ts(&event.timestamp);
	diff = (event.timestamp.tv_sec - vdev->ts_start.tv_sec) * 1000000;
	diff += (event.timestamp.tv_nsec - vdev->ts_start.tv_nsec) / 1000;
#endif
	*(__le32 *)(&event.u.data[1]) = cpu_to_le32(diff);

	if (type == V4L2_EVENT_CVI_VIP_VPSS_EARLY_INT)
		event.u.data[5] = sc_early_ids;

	event.u.data[6] = vdev->IntMask;
	*(__le32 *)(&event.u.data[7]) = cpu_to_le32(vdev->frame_number);
	v4l2_event_queue(&vdev->vdev, &event);
}

static int img_subscribe_event(struct v4l2_fh *fh,
	const struct v4l2_event_subscription *sub)
{
	if ((sub->type & V4L2_EVENT_CVI_VIP_CLASS) != V4L2_EVENT_CVI_VIP_CLASS)
		return -EINVAL;

	return v4l2_event_subscribe(fh, sub, CVI_VIP_IMG_MAX * 2, NULL);
}

static const struct v4l2_ioctl_ops cvi_img_ioctl_ops = {
	.vidioc_querycap = cvi_img_querycap,
	.vidioc_g_ctrl = cvi_img_g_ctrl,
	.vidioc_s_ctrl = cvi_img_s_ctrl,
	.vidioc_s_ext_ctrls = cvi_img_s_ext_ctrls,

	.vidioc_g_selection     = cvi_img_g_selection,
	//.vidioc_s_selection     = cvi_img_s_selection,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	.vidioc_enum_fmt_vid_cap = cvi_img_enum_fmt_vid_mplane,
#else
	.vidioc_enum_fmt_vid_cap_mplane = cvi_img_enum_fmt_vid_mplane,
#endif
	.vidioc_g_fmt_vid_cap_mplane    = cvi_img_g_fmt_vid_cap_mplane,
	.vidioc_try_fmt_vid_cap_mplane  = cvi_img_try_fmt_vid_cap_mplane,
	.vidioc_s_fmt_vid_cap_mplane    = cvi_img_s_fmt_vid_cap_mplane,

	.vidioc_reqbufs         = vb2_ioctl_reqbufs,
	//.vidioc_create_bufs     = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf     = vb2_ioctl_prepare_buf,
	.vidioc_querybuf        = vb2_ioctl_querybuf,
	.vidioc_qbuf            = vb2_ioctl_qbuf,
	.vidioc_dqbuf           = vb2_ioctl_dqbuf,
	//.vidioc_expbuf          = vb2_ioctl_expbuf,
	.vidioc_streamon        = cvi_img_streamon,
	.vidioc_streamoff       = cvi_img_streamoff,

	.vidioc_enum_input      = cvi_img_enum_input,
	.vidioc_g_input         = cvi_img_g_input,
	.vidioc_s_input         = cvi_img_s_input,
	.vidioc_subscribe_event     = img_subscribe_event,
	.vidioc_unsubscribe_event   = v4l2_event_unsubscribe,
};

/**
 * cvi_img_get_sc_bound() - to know which sc is bound with this img
 * @idev: the img-dev to check
 * @sc_bound: true if the sc-dev bound with idev
 */
void cvi_img_get_sc_bound(struct cvi_img_vdev *idev, bool sc_bound[])
{
	u8 i, sc_start_idx, chn_num;
	struct cvi_vpss_grp_cfg *grp_cfg;

	switch (idev->sc_bounding) {
	default:
	case CVI_VIP_IMG_2_SC_NONE:
		sc_start_idx = CVI_VIP_SC_D;
		chn_num = 0;
	break;

	case CVI_VIP_IMG_2_SC_D:
		sc_start_idx = CVI_VIP_SC_D;
		chn_num = SCL_D_MAX_INST;
	break;

	case CVI_VIP_IMG_2_SC_V:
		sc_start_idx = CVI_VIP_SC_V0;
		chn_num = SCL_V_MAX_INST;
	break;

	case CVI_VIP_IMG_2_SC_ALL:
		sc_start_idx = CVI_VIP_SC_D;
		chn_num = SCL_MAX_INST;
	break;
	}

	memset(sc_bound, false, sizeof(sc_bound[0]) * CVI_VIP_SC_MAX);
	if (idev->is_online_from_isp)
		grp_cfg = &idev->ol_vpss_grp_cfg[idev->job_grp];
	else
		grp_cfg = &idev->vpss_grp_cfg;
	for (i = 0; i < chn_num; ++i) {
		if (grp_cfg->chn_enable[i])
			sc_bound[i + sc_start_idx] = true;
	}
}

/**
 * cvi_img_device_run_work() - run pending jobs for the context
 * @data: data used for scheduling the execution of this function.
 */
static void cvi_img_device_run_work(unsigned long data)
{
	struct cvi_img_vdev *idev = (struct cvi_img_vdev *)data;

	cvi_vip_try_schedule(idev, 0, false);
}
/*************************************************************************
 *	General functions
 *************************************************************************/
int img_create_instance(struct platform_device *pdev)
{
	int rc = 0;
	struct cvi_vip_dev *bdev;
	struct video_device *vfd;
	struct cvi_img_vdev *idev;
	struct vb2_queue *q;
	u8 i = 0;

	bdev = dev_get_drvdata(&pdev->dev);
	if (!bdev) {
		dprintk(VIP_ERR, "invalid data\n");
		return -EINVAL;
	}

	for (i = 0; i < CVI_VIP_IMG_MAX; ++i) {
		idev = &bdev->img_vdev[i];
		mutex_init(&idev->mutex);

		idev->clk = devm_clk_get(&pdev->dev, CLK_IMG_NAME[i]);
		if (IS_ERR(idev->clk)) {
			pr_err("Cannot get clk for img-%d\n", i);
			idev->clk = NULL;
		}

		idev->align = VIP_ALIGNMENT;
		idev->dev_idx = i;
		idev->job_grp = 0;
		idev->img_type = (i == 0) ? SCL_IMG_D : SCL_IMG_V;
		idev->sc_bounding =
			(i == 0) ? CVI_VIP_IMG_2_SC_D : CVI_VIP_IMG_2_SC_V;
		idev->fmt = cvi_vip_get_format(V4L2_PIX_FMT_RGBM);
		idev->vid_caps = V4L2_CAP_VIDEO_CAPTURE_MPLANE |
			V4L2_CAP_STREAMING;
		idev->is_tile = false;
		idev->is_work_on_r_tile = true;
		idev->tile_mode = 0;
		idev->is_online_from_isp = false;
		idev->is_cmdq = false;
		idev->IntMask = 0;
		idev->retrigger_isp = false;
		atomic_set(&idev->is_streaming, 0);
		spin_lock_init(&idev->job_lock);
		memset(&idev->src_size, 0, sizeof(idev->src_size));
		memset(&idev->crop_rect, 0, sizeof(idev->crop_rect));
		memset(&idev->post_para, 0, sizeof(idev->post_para));

		vfd = &(idev->vdev);
		snprintf(vfd->name, sizeof(vfd->name), "cvi-img%d", i);
		vfd->fops = &cvi_img_fops;
		vfd->ioctl_ops = &cvi_img_ioctl_ops;
		vfd->vfl_dir = VFL_DIR_RX;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		vfd->vfl_type = VFL_TYPE_VIDEO;
#else
		vfd->vfl_type = VFL_TYPE_GRABBER;
#endif
		vfd->minor = -1;
		vfd->device_caps = idev->vid_caps;
		vfd->release = video_device_release_empty;
		vfd->v4l2_dev = &bdev->v4l2_dev;
		vfd->lock = &idev->mutex;
		vfd->queue = &idev->vb_q;

		// vb2_queue init
		q = &idev->vb_q;
		q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		q->io_modes = VB2_USERPTR;
		q->buf_struct_size = sizeof(struct cvi_vip_buffer);
		q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
		q->min_buffers_needed = 0;
		q->drv_priv = idev;
		q->dev = bdev->v4l2_dev.dev;
		q->ops = &cvi_img_qops;
		q->mem_ops = &cvi_img_vb2_mem_ops;
		//q->lock = &idev->lock;
		rc = vb2_queue_init(q);
		if (rc) {
			dprintk(VIP_ERR, "vb2_queue_init failed, ret=%d\n", rc);
			continue;
		}
		spin_lock_init(&idev->rdy_lock);
		INIT_LIST_HEAD(&idev->rdy_queue);
		idev->num_rdy = 0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		rc = video_register_device(vfd, VFL_TYPE_VIDEO,
				IMG_DEVICE_IDX + i);
#else
		rc = video_register_device(vfd, VFL_TYPE_GRABBER,
				IMG_DEVICE_IDX + i);
#endif
		if (rc) {
			dprintk(VIP_ERR, "Failed to register img-dev%d\n", i);
			continue;
		}
		video_set_drvdata(vfd, idev);
		idev->job_flags = 0;
		tasklet_init(&idev->job_work, cvi_img_device_run_work, (unsigned long)idev);
		tasklet_disable(&idev->job_work);

		dprintk(VIP_INFO, "img registered as %s\n",
				video_device_node_name(vfd));
	}

	memset(&g_sclr_rgnex_list, 0, sizeof(g_sclr_rgnex_list));
	sclr_cmdq_addr = kzalloc(0x2000, GFP_ATOMIC);
	if (!sclr_cmdq_addr) {
		dprintk(VIP_ERR, "sclr_cmdq_addr alloc fail\n");
		return -ENOMEM;
	}

	return rc;
}

int img_destroy_instance(struct platform_device *pdev)
{
	struct cvi_vip_dev *bdev;
	struct cvi_img_vdev *idev;
	u8 i = 0;

	bdev = dev_get_drvdata(&pdev->dev);
	if (!bdev) {
		dprintk(VIP_ERR, "invalid data\n");
		return -EINVAL;
	}

	for (i = 0; i < CVI_VIP_IMG_MAX; ++i) {
		idev = &bdev->img_vdev[i];
		tasklet_kill(&idev->job_work);
	}

	kfree(sclr_cmdq_addr);
	return 0;
}

void img_irq_handler(union sclr_intr intr_status, u8 cmdq_intr_status, struct cvi_vip_dev *bdev)
{
	u8 img_idx = 0;
	struct cvi_img_vdev *idev = NULL;

	if (cmdq_intr_status & 0x02) {
		// currently only img_d use cmdq when doing rgn_ex job
		if (bdev->img_vdev[0].is_cmdq == true)
			cvi_img_v4l2_event_queue(&bdev->img_vdev[0],
				V4L2_EVENT_CVI_VIP_VPSS_CMDQ_DONE, 0, 0);
	}

	for (img_idx = 0; img_idx < CVI_VIP_IMG_MAX; ++img_idx) {
		struct cvi_vip_buffer *img_b = NULL;

		// check if frame_done
		if (((img_idx == CVI_VIP_IMG_D) &&
				(intr_status.b.img_in_d_frame_end == 0 || bdev->img_vdev[img_idx].is_cmdq == true)) ||
		    ((img_idx == CVI_VIP_IMG_V) &&
				(intr_status.b.img_in_v_frame_end == 0)))
			continue;

		idev = &bdev->img_vdev[img_idx];
		idev->job_flags &= ~(TRANS_RUNNING);
		dprintk(VIP_DBG, "img-%d: grp(%d) frame_end\n", img_idx, idev->job_grp);
		// in tile mode, only step forward if right-tile is done.
		if (idev->is_tile) {
			if (!idev->is_work_on_r_tile)
				idev->tile_mode &= ~(SCL_TILE_LEFT);
			else
				idev->tile_mode &= ~(SCL_TILE_RIGHT);

			if (idev->tile_mode != 0) {
				cvi_vip_job_finish(idev);
				continue;
			}
		}

		if (idev->is_online_from_isp)
			idev->ol_irq_cnt[idev->job_grp]++;
		else
			idev->irq_cnt++;
		++idev->frame_number;

		dprintk(VIP_DBG, "img_idx(%d) input_type(%d) disp_online(%d)\n"
			, img_idx, idev->input_type, bdev->disp_online);
		// if input isn't memory, don't care buffer.
		if (idev->input_type != CVI_VIP_INPUT_MEM) {
			cvi_vip_job_finish(idev);
			continue;
		}

		// check buf-num if online to disp
		if ((img_idx == CVI_VIP_IMG_D) && bdev->disp_online)
			if (idev->num_rdy <= 1)
				continue;

		img_b = cvi_vip_buf_remove((struct cvi_base_vdev *)idev);
		if (!img_b) {
			dprintk(VIP_ERR, "no img%d buf, intr-status(%#x)\n",
					img_idx, intr_status.raw);
			continue;
		}

		// update vb2's info
		img_b->vb.vb2_buf.timestamp = ktime_get_ns();
		img_b->vb.sequence = ++idev->seq_count;

		vb2_buffer_done(&img_b->vb.vb2_buf, VB2_BUF_STATE_DONE);

		// update job-flag and see if there are other jobs
		cvi_vip_job_finish(idev);
	}
}

void cvi_img_update(struct cvi_img_vdev *idev, const struct cvi_vpss_grp_cfg *grp_cfg)
{
	const struct cvi_vip_fmt *fmt;
	struct sclr_img_cfg *cfg;

	fmt = cvi_vip_get_format(grp_cfg->pixelformat);
	cfg = sclr_img_get_cfg(idev->img_type);

	if (idev->is_online_from_isp) {
		cfg->fmt = SCL_FMT_YUV422;
		cfg->csc = SCL_CSC_601_LIMIT_YUV2RGB;
	} else {
		cfg->fmt = fmt->fmt;
		if (grp_cfg->pixelformat == V4L2_PIX_FMT_YUV444M)
			cfg->csc = SCL_CSC_601_LIMIT_YUV2RGB;
		else if (IS_YUV_FMT(cfg->fmt))
			cfg->csc = (cfg->fmt == SCL_FMT_Y_ONLY) ? SCL_CSC_NONE : SCL_CSC_601_LIMIT_YUV2RGB;
		else
			cfg->csc = SCL_CSC_NONE;
	}
	cfg->mem.pitch_y = grp_cfg->bytesperline[0];
	cfg->mem.pitch_c = grp_cfg->bytesperline[1];

	cfg->mem.start_x = grp_cfg->crop.left;
	cfg->mem.start_y = grp_cfg->crop.top;
	cfg->mem.width	 = grp_cfg->crop.width;
	cfg->mem.height  = grp_cfg->crop.height;

	sclr_img_set_cfg(idev->img_type, cfg);

	sclr_img_set_csc(idev->img_type, (struct sclr_csc_matrix *)&grp_cfg->csc_cfg);
}
