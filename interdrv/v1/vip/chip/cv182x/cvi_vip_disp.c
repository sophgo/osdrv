/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_vip_disp.c
 * Description: video pipeline display driver
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/videodev2.h>
#include <linux/v4l2-dv-timings.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/of_gpio.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-common.h>
#include <media/v4l2-dv-timings.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-event.h>
#include <media/v4l2-rect.h>

#include "vip/vip_common.h"
#include "vip/scaler.h"
#include "vip/dsi_phy.h"
#include "vip/reg.h"

#include "cvi_debug.h"
#include "cvi_vip_core.h"
#include "cvi_vip_disp.h"
#include "cvi_vip_mipi_tx.h"
#include "cvi_vip_vo_proc.h"

#include "pinctrl-cv1822.h"

#define MAX_DISP_WIDTH  1920
#define MAX_DISP_HEIGHT 1080
#define VO_SHARE_MEM_SIZE (0x1000)
#define DISP_PROC_NAME "cvitek/vo"

static const char *const CLK_DISP_NAME = "clk_disp";
static const char *const CLK_BT_NAME = "clk_bt";
static const char *const CLK_DSI_NAME = "clk_dsi";

// for proc info
static int proc_vo_mode;
static const char * const str_sclr_fmt[] = {"YUV420", "YUV422", "RGB_PLANAR", "RGB_PACKED", "BGR_PACKED", "Y_ONLY"};
static const char * const str_sclr_csc[] = {"Disable", "2RGB_601_Limit", "2RGB_601_Full", "2RGB_709_Limit"
	, "2RGB_709_Full", "2YUV_601_Limit", "2YUV_601_Full", "2YUV_709_Limit", "2YUV_709_Full"};

const struct v4l2_dv_timings_cap cvi_disp_dv_timings_caps = {
	.type = V4L2_DV_BT_656_1120,
	/* keep this initialization for compatibility with GCC < 4.4.6 */
	.reserved = { 0 },
	V4L2_INIT_BT_TIMINGS(0, MAX_DISP_WIDTH, 0, MAX_DISP_HEIGHT,
		14000000, 148500000,
		V4L2_DV_BT_STD_CEA861 | V4L2_DV_BT_STD_DMT |
		V4L2_DV_BT_STD_CVT | V4L2_DV_BT_STD_GTF,
		V4L2_DV_BT_CAP_PROGRESSIVE)
};

const struct v4l2_dv_timings def_dv_timings[] = {
	{   // TTL for FPGA
		.type = V4L2_DV_BT_656_1120,
		V4L2_INIT_BT_TIMINGS(800, 600, 0, V4L2_DV_HSYNC_POS_POL,
			  27000000, 16, 9, 5, 1, 3, 24, 0, 0, 0,
			  V4L2_DV_BT_STD_CEA861)
	},
	V4L2_DV_BT_CEA_1920X1080P25,
	V4L2_DV_BT_CEA_1920X1080P30,
	V4L2_DV_BT_CEA_1920X1080I50,
	V4L2_DV_BT_CEA_1920X1080P50,
	V4L2_DV_BT_CEA_1920X1080I60,
	V4L2_DV_BT_CEA_1920X1080P60,
};

struct cvi_vip_disp_pattern {
	enum sclr_disp_pat_type type;
	enum sclr_disp_pat_color color;
	u16 rgb[3];
};

const struct cvi_vip_disp_pattern patterns[CVI_VIP_PAT_MAX] = {
	{.type = SCL_PAT_TYPE_OFF,	.color = SCL_PAT_COLOR_MAX},
	{.type = SCL_PAT_TYPE_SNOW,	.color = SCL_PAT_COLOR_MAX},
	{.type = SCL_PAT_TYPE_AUTO,	.color = SCL_PAT_COLOR_MAX},
	{.type = SCL_PAT_TYPE_FULL,	.color = SCL_PAT_COLOR_RED},
	{.type = SCL_PAT_TYPE_FULL,	.color = SCL_PAT_COLOR_GREEN},
	{.type = SCL_PAT_TYPE_FULL,	.color = SCL_PAT_COLOR_BLUE},
	{.type = SCL_PAT_TYPE_FULL,	.color = SCL_PAT_COLOR_BAR},
	{.type = SCL_PAT_TYPE_H_GRAD,   .color = SCL_PAT_COLOR_WHITE},
	{.type = SCL_PAT_TYPE_V_GRAD,   .color = SCL_PAT_COLOR_WHITE},
	{.type = SCL_PAT_TYPE_FULL,     .color = SCL_PAT_COLOR_USR,        .rgb = {0, 0, 0} },
};

int smooth;
struct platform_device *g_pdev;

static bool hide_vo;
module_param(hide_vo, bool, 0444);

static void _hw_enque(struct cvi_disp_vdev *ddev)
{
	struct vb2_buffer *vb2_buf;
	struct cvi_vip_buffer *b = NULL;
	struct cvi_vip_dev *bdev =
		container_of(ddev, struct cvi_vip_dev, disp_vdev);
	struct sclr_disp_cfg *cfg;

	if (!ddev || bdev->disp_online)
		return;

	b = cvi_vip_next_buf((struct cvi_base_vdev *)ddev);
	if (!b)
		return;
	vb2_buf = &b->vb.vb2_buf;

	dprintk(VIP_DBG, "update disp-buf: 0x%lx-0x%lx-0x%lx\n",
		vb2_buf->planes[0].m.userptr, vb2_buf->planes[1].m.userptr,
		vb2_buf->planes[2].m.userptr);

	cfg = sclr_disp_get_cfg();
	cfg->mem.addr0 = vb2_buf->planes[0].m.userptr;
	cfg->mem.addr1 = vb2_buf->planes[1].m.userptr;
	cfg->mem.addr2 = vb2_buf->planes[2].m.userptr;
	cfg->mem.pitch_y = (vb2_buf->planes[0].bytesused > ddev->bytesperline[0])
			 ? vb2_buf->planes[0].bytesused
			 : ddev->bytesperline[0];
	cfg->mem.pitch_c = (vb2_buf->planes[1].bytesused > ddev->bytesperline[1])
			 ? vb2_buf->planes[1].bytesused
			 : ddev->bytesperline[1];
	sclr_disp_set_mem(&cfg->mem);
	if (ddev->disp_interface == CVI_VIP_DISP_INTF_I80) {
		sclr_disp_reg_force_up();
		sclr_i80_run();
	}
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
static int cvi_disp_queue_setup(struct vb2_queue *vq,
		unsigned int *nbuffers, unsigned int *nplanes,
		unsigned int sizes[], struct device *alloc_devs[])
{
	struct cvi_disp_vdev *ddev = vb2_get_drv_priv(vq);
	unsigned int planes = ddev->fmt->buffers;
	unsigned int p;

	dprintk(VIP_VB2, "+\n");

	for (p = 0; p < planes; ++p)
		sizes[p] = ddev->sizeimage[p];

	if (vq->num_buffers + *nbuffers < 2)
		*nbuffers = 2 - vq->num_buffers;

	*nplanes = ddev->fmt->buffers;

	dprintk(VIP_INFO, "num_buffer=%d, num_plane=%d\n", *nbuffers, *nplanes);
	for (p = 0; p < *nplanes; p++)
		dprintk(VIP_INFO, "size[%u]=%u\n", p, sizes[p]);

	return 0;
}

/**
 * for VIDIOC_STREAMON, start fill data.
 */
static void cvi_disp_buf_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct cvi_disp_vdev *ddev = vb2_get_drv_priv(vb->vb2_queue);
	struct cvi_vip_buffer *cvi_vb2 =
		container_of(vbuf, struct cvi_vip_buffer, vb);

	dprintk(VIP_VB2, "+\n");

	cvi_vip_buf_queue((struct cvi_base_vdev *)ddev, cvi_vb2);

	if (ddev->num_rdy == 1) {
		_hw_enque(ddev);
		if (ddev->disp_interface == CVI_VIP_DISP_INTF_I80) {
			cvi_vip_buf_remove((struct cvi_base_vdev *)ddev);
			vb2_buffer_done(vb, VB2_BUF_STATE_DONE);
		}
	}
}

static int cvi_disp_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct cvi_disp_vdev *ddev = vb2_get_drv_priv(vq);
	struct sclr_top_cfg *cfg = sclr_top_get_cfg();
	int rc = 0;

	dprintk(VIP_VB2, "+\n");

	cfg->disp_enable = true;
	sclr_top_set_cfg(cfg);
	sclr_disp_enable_window_bgcolor(true);

	ddev->align = VIP_ALIGNMENT;
	ddev->seq_count = 0;
	ddev->frame_number = 0;
	if (ddev->disp_interface != CVI_VIP_DISP_INTF_I80)
		sclr_disp_tgen_enable(true);
	return rc;
}

/* abort streaming and wait for last buffer */
static void cvi_disp_stop_streaming(struct vb2_queue *vq)
{
	struct cvi_disp_vdev *ddev = vb2_get_drv_priv(vq);
	struct cvi_vip_buffer *cvi_vb2, *tmp;
	unsigned long flags;
	struct vb2_buffer *vb2_buf;
	struct sclr_top_cfg *cfg = sclr_top_get_cfg();

	cfg->disp_enable = false;
	sclr_top_set_cfg(cfg);

	sclr_disp_enable_window_bgcolor(true);
	dprintk(VIP_VB2, "+\n");

	if (!smooth && (ddev->disp_interface != CVI_VIP_DISP_INTF_LVDS))
		sclr_disp_tgen_enable(false);

	/*
	 * Release all the buffers enqueued to driver
	 * when streamoff is issued
	 */
	spin_lock_irqsave(&ddev->rdy_lock, flags);
	list_for_each_entry_safe(cvi_vb2, tmp, &(ddev->rdy_queue), list) {
		vb2_buf = &(cvi_vb2->vb.vb2_buf);
		if (vb2_buf->state == VB2_BUF_STATE_DONE)
			continue;
		vb2_buffer_done(vb2_buf, VB2_BUF_STATE_DONE);
	}
	ddev->num_rdy = 0;
	INIT_LIST_HEAD(&ddev->rdy_queue);
	spin_unlock_irqrestore(&ddev->rdy_lock, flags);

	memset(&ddev->compose_out, 0, sizeof(ddev->compose_out));
}

const struct vb2_ops cvi_disp_qops = {
//    .buf_init           =
	.queue_setup        = cvi_disp_queue_setup,
//    .buf_finish         = cvi_disp_buf_finish,
	.buf_queue          = cvi_disp_buf_queue,
	.start_streaming    = cvi_disp_start_streaming,
	.stop_streaming     = cvi_disp_stop_streaming,
//    .wait_prepare       = vb2_ops_wait_prepare,
//    .wait_finish        = vb2_ops_wait_finish,
};

/*************************************************************************
 *	VB2-MEM-OPS definition
 *************************************************************************/
static void *disp_get_userptr(struct device *dev, unsigned long vaddr,
	unsigned long size, enum dma_data_direction dma_dir)
{
	return (void *)0xdeadbeef;
}

static void disp_put_userptr(void *buf_priv)
{
}

static const struct vb2_mem_ops cvi_disp_vb2_mem_ops = {
	.get_userptr = disp_get_userptr,
	.put_userptr = disp_put_userptr,
};

/*************************************************************************
 *	FOPS definition
 *************************************************************************/
static int cvi_disp_open(struct file *file)
{
	int rc = 0;
	struct cvi_disp_vdev *ddev = video_drvdata(file);

	WARN_ON(!ddev);

	rc = v4l2_fh_open(file);
	if (rc) {
		dprintk(VIP_ERR, "v4l2_fh_open failed(%d)\n", rc);
		return rc;
	}

	if (v4l2_fh_is_singular_file(file)) {
		struct cvi_vip_dev *bdev = container_of(ddev, struct cvi_vip_dev, disp_vdev);

		if (bdev->clk_sc_top)
			clk_prepare_enable(bdev->clk_sc_top);
		if (ddev->clk_disp)
			clk_prepare_enable(ddev->clk_disp);

		sclr_disp_reg_shadow_sel(false);
		if (!smooth)
			sclr_disp_set_cfg(sclr_disp_get_cfg());
	}

	dprintk(VIP_INFO, "by %s\n", current->comm);
	return rc;
}

static int cvi_disp_release(struct file *file)
{
	struct cvi_disp_vdev *ddev = video_drvdata(file);

	WARN_ON(!ddev);

	if (vb2_is_streaming(&ddev->vb_q))
		vb2_streamoff(&ddev->vb_q, ddev->vb_q.type);

	if (v4l2_fh_is_singular_file(file)) {
		struct cvi_vip_dev *bdev = container_of(ddev, struct cvi_vip_dev, disp_vdev);

		if (!(debug & BIT(2)) && ddev->clk_disp && __clk_is_enabled(ddev->clk_disp))
			clk_disable_unprepare(ddev->clk_disp);
		if (!(debug & BIT(2)) && bdev->clk_sc_top && __clk_is_enabled(bdev->clk_sc_top))
			clk_disable_unprepare(bdev->clk_sc_top);
	}

	vb2_fop_release(file);

	dprintk(VIP_INFO, "-\n");
	return 0;
}

#if 0
static unsigned int cvi_disp_poll(struct file *file,
	struct poll_table_struct *wait)
{
	struct cvi_disp_vdev *ddev = video_drvdata(file);
	struct vb2_buffer *out_vb = NULL;
	unsigned long flags;
	int rc = 0;

	WARN_ON(!ddev);

	poll_wait(file, &ddev->vb_q.done_wq, wait);
	rc = vb2_fop_poll(file, wait);
	spin_lock_irqsave(&ddev->rdy_lock, flags);
	if (!list_empty(&ddev->vb_q.done_list))
	out_vb = list_first_entry(&ddev->vb_q.done_list, struct vb2_buffer,
				  done_entry);
	if (out_vb && (out_vb->state == VB2_BUF_STATE_DONE
		|| out_vb->state == VB2_BUF_STATE_ERROR))
	rc |= POLLOUT | POLLWRNORM;
	spin_unlock_irqrestore(&ddev->rdy_lock, flags);
	return rc;
}
#endif

static int disp_subscribe_event(struct v4l2_fh *fh,
	const struct v4l2_event_subscription *sub)
{
	if (sub->type != V4L2_EVENT_FRAME_SYNC)
		return -EINVAL;

	return v4l2_event_subscribe(fh, sub, CVI_DISP_NEVENTS, NULL);
}

static int cvi_disp_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct cvi_disp_vdev *ddev = video_drvdata(file);
	unsigned long vm_start = vma->vm_start;
	unsigned int vm_size = vma->vm_end - vma->vm_start;
	unsigned int offset = vma->vm_pgoff << PAGE_SHIFT;
	void *pos = ddev->shared_mem;

	if (offset < 0 || (vm_size + offset) > VO_SHARE_MEM_SIZE)
		return -EINVAL;

	while (vm_size > 0) {
		if (remap_pfn_range(vma, vm_start, virt_to_pfn(pos), PAGE_SIZE, vma->vm_page_prot))
			return -EAGAIN;
		pr_debug("vo proc mmap vir(%p) phys(%#llx)\n", pos, virt_to_phys((void *) pos));
		vm_start += PAGE_SIZE;
		pos += PAGE_SIZE;
		vm_size -= PAGE_SIZE;
	}

	return 0;
}

static struct v4l2_file_operations cvi_disp_fops = {
	.owner = THIS_MODULE,
	.open = cvi_disp_open,
	.release = cvi_disp_release,
	.mmap = cvi_disp_mmap,
	.poll = vb2_fop_poll, //.poll = cvi_disp_poll,
	.unlocked_ioctl = video_ioctl2,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = video_ioctl2,
#endif
};

/*************************************************************************
 *	IOCTL definition
 *************************************************************************/
static int cvi_disp_querycap(struct file *file, void *priv,
			struct v4l2_capability *cap)
{
	struct cvi_disp_vdev *ddev = video_drvdata(file);
	struct cvi_vip_dev *bdev =
		container_of(ddev, struct cvi_vip_dev, disp_vdev);

	strlcpy(cap->driver, CVI_VIP_DRV_NAME, sizeof(cap->driver));
	strlcpy(cap->card, CVI_VIP_DVC_NAME, sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info),
		"platform:%s", bdev->v4l2_dev.name);

	cap->capabilities = ddev->vid_caps | V4L2_CAP_DEVICE_CAPS;
	return 0;
}

static int cvi_disp_g_ctrl(struct file *file, void *priv,
		struct v4l2_control *vc)
{
	int rc = -EINVAL;
	return rc;
}

static int cvi_disp_s_ctrl(struct file *file, void *priv,
		struct v4l2_control *vc)
{
	int rc = -EINVAL;
	return rc;
}

static void _disp_sel_pinmux(enum cvi_disp_intf intf_type, void *param)
{
	if (intf_type == CVI_VIP_DISP_INTF_I80) {
		PINMUX_CONFIG(PAD_MIPIRX2N, VO_D_10);
		PINMUX_CONFIG(PAD_MIPIRX2P, VO_D_9);
		PINMUX_CONFIG(PAD_MIPIRX1N, VO_D_8);
		PINMUX_CONFIG(PAD_MIPIRX1P, VO_D_7);
		PINMUX_CONFIG(PAD_MIPIRX0N, VO_D_6);
		PINMUX_CONFIG(PAD_MIPIRX0P, VO_D_5);
		PINMUX_CONFIG(PAD_MIPI_TXM2, VO_D_0);
		PINMUX_CONFIG(PAD_MIPI_TXP2, VO_CLK0);
		PINMUX_CONFIG(PAD_MIPI_TXM1, VO_D_2);
		PINMUX_CONFIG(PAD_MIPI_TXP1, VO_D_1);
		PINMUX_CONFIG(PAD_MIPI_TXM0, VO_D_4);
		PINMUX_CONFIG(PAD_MIPI_TXP0, VO_D_3);
	} else if (intf_type == CVI_VIP_DISP_INTF_LVDS) {
		PINMUX_CONFIG(PAD_MIPI_TXM0, XGPIOC_12);
		PINMUX_CONFIG(PAD_MIPI_TXP0, XGPIOC_13);
		PINMUX_CONFIG(PAD_MIPI_TXM1, XGPIOC_14);
		PINMUX_CONFIG(PAD_MIPI_TXP1, XGPIOC_15);
		PINMUX_CONFIG(PAD_MIPI_TXM2, XGPIOC_16);
		PINMUX_CONFIG(PAD_MIPI_TXP2, XGPIOC_17);
		PINMUX_CONFIG(PAD_MIPI_TXM3, XGPIOC_20);
		PINMUX_CONFIG(PAD_MIPI_TXP3, XGPIOC_21);
		PINMUX_CONFIG(PAD_MIPI_TXM4, XGPIOC_18);
		PINMUX_CONFIG(PAD_MIPI_TXP4, XGPIOC_19);
	} else if (intf_type == CVI_VIP_DISP_INTF_BT) {
		struct cvi_bt_intf_cfg *cfg = param;

		if (cfg->mode == BT_MODE_601) {
			PINMUX_CONFIG(PAD_MIPIRX2N, VO_D_10);
			PINMUX_CONFIG(PAD_MIPIRX2P, VO_D_9);
			PINMUX_CONFIG(PAD_MIPIRX1N, VO_D_8);
		}
		PINMUX_CONFIG(PAD_MIPIRX1P, VO_D_7);
		PINMUX_CONFIG(PAD_MIPIRX0N, VO_D_6);
		PINMUX_CONFIG(PAD_MIPIRX0P, VO_D_5);
		PINMUX_CONFIG(PAD_MIPI_TXM2, VO_D_0);
		PINMUX_CONFIG(PAD_MIPI_TXP2, VO_CLK0);
		PINMUX_CONFIG(PAD_MIPI_TXM1, VO_D_2);
		PINMUX_CONFIG(PAD_MIPI_TXP1, VO_D_1);
		PINMUX_CONFIG(PAD_MIPI_TXM0, VO_D_4);
		PINMUX_CONFIG(PAD_MIPI_TXP0, VO_D_3);
	}
}

static void _disp_ctrlpin_set(unsigned int gpio_num, GPIO_ACTIVE_E active)
{
	enum of_gpio_flags flags;
	static int count;
	char name[16] = "";
	int rc = 0;

	if (gpio_is_valid(gpio_num)) {
		flags = GPIOF_DIR_OUT | (active ? GPIOF_INIT_HIGH : GPIOF_INIT_LOW);
		snprintf(name, sizeof(name), "disp_ctrl_pin_%d", count++);
		rc = devm_gpio_request_one(&g_pdev->dev, gpio_num, flags, name);
		if (rc) {
			dprintk(VIP_ERR, "(%s)) gpio_num(%d) failed\n", __func__, gpio_num);
			return;
		}
		gpio_set_value(gpio_num, active);
	}
}

static int cvi_disp_s_ext_ctrls(struct file *file, void *priv,
	struct v4l2_ext_controls *vc)
{
	struct cvi_disp_vdev *ddev = video_drvdata(file);
	struct v4l2_ext_control *ext_ctrls;
	union sclr_intr intr_mask;
	int rc = -EINVAL, i = 0;

	ext_ctrls = vc->controls;
	for (i = 0; i < vc->count; ++i) {
		switch (ext_ctrls[i].id) {
		case V4L2_CID_DV_VIP_DISP_INTR: {
			static bool service_isr = true;

			service_isr = !service_isr;
			dprintk(VIP_DBG, "service_irs(%d)\n", service_isr);
			intr_mask = sclr_get_intr_mask();
			intr_mask.b.disp_frame_end = (service_isr) ? 1 : 0;
			sclr_set_intr_mask(intr_mask);
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_DISP_OUT_CSC:
			if (ext_ctrls[i].value >= SCL_CSC_601_LIMIT_YUV2RGB &&
			    ext_ctrls[i].value <= SCL_CSC_709_FULL_YUV2RGB) {
				dprintk(VIP_ERR, "invalid disp-out-csc(%d)\n", ext_ctrls[i].value);
				break;
			}
			sclr_disp_set_out_csc(ext_ctrls[i].value);
			rc = 0;
		break;

		case V4L2_CID_DV_VIP_DISP_PATTERN:
			if (ext_ctrls[i].value >= CVI_VIP_PAT_MAX) {
				dprintk(VIP_ERR, "invalid disp-pattern(%d)\n",
						ext_ctrls[i].value);
				break;
			}
			sclr_disp_set_pattern(patterns[ext_ctrls[i].value].type,
					patterns[ext_ctrls[i].value].color, patterns[ext_ctrls[i].value].rgb);
			rc = 0;
		break;

		case V4L2_CID_DV_VIP_DISP_FRAME_BGCOLOR: {
			u16 r, g, b;

			r = *ext_ctrls[i].p_u16;
			g = *(ext_ctrls[i].p_u16 + 1);
			b = *(ext_ctrls[i].p_u16 + 2);
			sclr_disp_set_frame_bgcolor(r, g, b);
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_DISP_WINDOW_BGCOLOR: {
			u16 r, g, b;

			r = *ext_ctrls[i].p_u16;
			g = *(ext_ctrls[i].p_u16 + 1);
			b = *(ext_ctrls[i].p_u16 + 2);
			sclr_disp_set_window_bgcolor(r, g, b);
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_DISP_ONLINE: {
			struct cvi_vip_dev *bdev = NULL;

			if (vb2_is_streaming(&ddev->vb_q)) {
				dprintk(VIP_ERR, "V4L2_CID_DV_VIP_DISP_ONLINE can't be control if streaming.\n");
				break;
			}

			bdev = container_of(ddev, struct cvi_vip_dev, disp_vdev);
			bdev->disp_online = ext_ctrls[i].value;
			sclr_ctrl_set_disp_src(bdev->disp_online);
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_DISP_INTF: {
			struct cvi_disp_intf_cfg *cfg, _cfg_;

			cfg = &_cfg_;
			if (copy_from_user(cfg, ext_ctrls[i].ptr, sizeof(struct cvi_disp_intf_cfg))) {
				dprintk(VIP_ERR, "ioctl-%#x, copy_from_user failed.\n", ext_ctrls[i].id);
				break;
			}

			if (smooth) {
				dprintk(VIP_ERR, "V4L2_CID_DV_VIP_DISP_INTF won't apply if smooth.\n");
				sclr_disp_reg_force_up();
				ddev->disp_interface = cfg->intf_type;
				rc = 0;
				break;
			}

			if (vb2_is_streaming(&ddev->vb_q)) {
				dprintk(VIP_ERR, "V4L2_CID_DV_VIP_DISP_INTF can't be control if streaming.\n");
				break;
			}

			if (cfg->intf_type == CVI_VIP_DISP_INTF_DSI) {
				dprintk(VIP_ERR, "MIPI use mipi_tx to control.\n");
				return rc;
			} else if (cfg->intf_type == CVI_VIP_DISP_INTF_LVDS) {
				union sclr_lvdstx lvds_reg;
				bool data_en[LANE_MAX_NUM] = {false, false, false, false, false};
				struct clk *clk_disp, *clk_dsi;

				clk_disp = devm_clk_get(&g_pdev->dev, CLK_DISP_NAME);
				if (IS_ERR(clk_disp)) {
					dprintk(VIP_ERR, "devm_clk_get clk_disp failed.\n");
					return rc;
				}
				if (clk_disp)
					clk_prepare_enable(clk_disp);

				clk_dsi = devm_clk_get(&g_pdev->dev, CLK_DSI_NAME);
				if (IS_ERR(clk_dsi)) {
					dprintk(VIP_ERR, "devm_clk_get clk_dsi failed.\n");
					return rc;
				}
				if (clk_dsi)
					clk_prepare_enable(clk_dsi);

				for (i = 0; i < LANE_MAX_NUM; i++) {
					if ((cfg->lvds_cfg.lane_id[i] < 0) ||
					    (cfg->lvds_cfg.lane_id[i] >= LANE_MAX_NUM)) {
						dphy_dsi_set_lane(i, DSI_LANE_MAX, false, false);
						continue;
					}
					dphy_dsi_set_lane(i, cfg->lvds_cfg.lane_id[i],
							  cfg->lvds_cfg.lane_pn_swap[i], false);
					if (cfg->lvds_cfg.lane_id[i] != MIPI_TX_LANE_CLK) {
						data_en[cfg->lvds_cfg.lane_id[i] - 1] = true;
					}
				}
				dphy_dsi_lane_en(true, data_en, false);
				_disp_sel_pinmux(cfg->intf_type, &cfg->lvds_cfg);
				_disp_ctrlpin_set(cfg->lvds_cfg.backlight_gpio_num, cfg->lvds_cfg.backlight_avtive);
				sclr_disp_set_intf(SCLR_VO_INTF_LVDS);

				if (cfg->lvds_cfg.pixelclock == 0) {
					dprintk(VIP_ERR, "lvds pixelclock 0 invalid\n");
					return rc;
				}
				lvds_reg.b.out_bit = cfg->lvds_cfg.out_bits;
				lvds_reg.b.vesa_mode = cfg->lvds_cfg.mode;
				if (cfg->lvds_cfg.chn_num == 1)
					lvds_reg.b.dual_ch = 0;
				else if (cfg->lvds_cfg.chn_num == 2)
					lvds_reg.b.dual_ch = 1;
				else {
					lvds_reg.b.dual_ch = 0;
					dprintk(VIP_WARN, "invalid lvds chn_num(%d). Use 1 instead."
						, cfg->lvds_cfg.chn_num);
				}
				lvds_reg.b.vs_out_en = cfg->lvds_cfg.vs_out_en;
				lvds_reg.b.hs_out_en = cfg->lvds_cfg.hs_out_en;
				lvds_reg.b.hs_blk_en = cfg->lvds_cfg.hs_blk_en;
				lvds_reg.b.ml_swap = cfg->lvds_cfg.msb_lsb_data_swap;
				lvds_reg.b.ctrl_rev = cfg->lvds_cfg.serial_msb_first;
				lvds_reg.b.oe_swap = cfg->lvds_cfg.even_odd_link_swap;
				lvds_reg.b.en = cfg->lvds_cfg.enable;
				dphy_lvds_set_pll(cfg->lvds_cfg.pixelclock, cfg->lvds_cfg.chn_num);
				dphy_dsi_analog_setting(true);
				sclr_lvdstx_set(lvds_reg);
			} else if (cfg->intf_type == CVI_VIP_DISP_INTF_I80) {
				union sclr_bt_enc enc;
				union sclr_bt_sync_code sync;

				_disp_sel_pinmux(cfg->intf_type, &cfg->bt_cfg);
				sclr_disp_set_intf(SCLR_VO_INTF_I80);
				enc.raw = 0;
				enc.b.fmt_sel = 2;
				enc.b.clk_inv = 1;
				sync.raw = 0;
				sync.b.sav_vld = 0x80;
				sync.b.sav_blk = 0xab;
				sync.b.eav_vld = 0x9d;
				sync.b.eav_blk = 0xb6;
				sclr_bt_set(enc, sync);
			} else if (cfg->intf_type == CVI_VIP_DISP_INTF_BT) {
				union sclr_bt_enc enc;

				if (cfg->bt_cfg.mode == BT_MODE_656)
					sclr_disp_set_intf(SCLR_VO_INTF_BT656);
				else if (cfg->bt_cfg.mode == BT_MODE_601)
					sclr_disp_set_intf(SCLR_VO_INTF_BT601);
				else {
					dprintk(VIP_ERR, "invalid bt-mode(%d)\n", cfg->bt_cfg.mode);
					return rc;
				}

				_disp_sel_pinmux(cfg->intf_type, &cfg->bt_cfg);
				enc.raw = 0;
				enc.b.fmt_sel = cfg->bt_cfg.mode;
			} else {
				dprintk(VIP_ERR, "invalid disp-intf(%d)\n", cfg->intf_type);
				return rc;
			}
			sclr_disp_reg_force_up();

			ddev->disp_interface = cfg->intf_type;
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_DISP_ENABLE_WIN_BGCOLOR: {
			ddev->bgcolor_enable = ext_ctrls[i].value;
			sclr_disp_enable_window_bgcolor(ext_ctrls[i].value);
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_DISP_SET_ALIGN: {
			if (ext_ctrls[i].value >= VIP_ALIGNMENT) {
				ddev->align = ext_ctrls[i].value;
				rc = 0;
			}
		}
		break;

		case V4L2_CID_DV_VIP_DISP_SET_RGN: {
			struct sclr_disp_timing *timing = sclr_disp_get_timing();
			struct sclr_size size;
			struct cvi_rgn_cfg cfg;

			if (copy_from_user(&cfg, ext_ctrls[i].ptr, sizeof(struct cvi_rgn_cfg))) {
				dprintk(VIP_ERR, "ioctl-%#x, copy_from_user failed.\n", ext_ctrls[i].id);
				break;
			}

			size.w = timing->hfde_end - timing->hfde_start + 1;
			size.h = timing->vfde_end - timing->vfde_start + 1;
			rc = cvi_vip_set_rgn_cfg(SCL_GOP_DISP, &cfg, &size);
		}
		break;

		case V4L2_CID_DV_VIP_DISP_SET_CUSTOM_CSC: {
			struct sclr_csc_matrix cfg;

			if (copy_from_user(&cfg, ext_ctrls[i].ptr, sizeof(struct sclr_csc_matrix))) {
				dprintk(VIP_ERR, "ioctl-%#x, copy_from_user failed.\n", ext_ctrls[i].id);
				break;
			}
			sclr_disp_set_csc(&cfg);

			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_DISP_I80_SW_MODE: {
			sclr_i80_sw_mode(ext_ctrls[i].value);
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_DISP_I80_CMD: {
			sclr_i80_packet(ext_ctrls[i].value);
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_DISP_SET_CLK: {
			if (ext_ctrls[i].value < 8000) {
				dprintk(VIP_ERR, "V4L2_CID_DV_VIP_DISP_SET_CLK clk(%d) less than 8000 kHz.\n",
					ext_ctrls[i].value);
				break;
			}
			dphy_dsi_set_pll(ext_ctrls[i].value, 4, 24);
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_DISP_GET_VLAYER_SIZE: {
			struct sclr_disp_timing *timing = sclr_disp_get_timing();
			struct dsize {
				u32 width;
				u32 height;
			} vsize;

			vsize.width = timing->hfde_end - timing->hfde_start + 1;
			vsize.height = timing->vfde_end - timing->vfde_start + 1;

			if (copy_to_user(ext_ctrls[i].ptr, &vsize, sizeof(struct dsize)))
				break;
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_DISP_GET_PANEL_STATUS: {
			int is_init = 0;

			if (sclr_disp_mux_get() == SCLR_VO_SEL_I80) {
				is_init = sclr_disp_check_i80_enable();
			} else {
				is_init = sclr_disp_check_tgen_enable();
			}
			if (copy_to_user(ext_ctrls[i].ptr, &is_init, sizeof(is_init)))
				break;
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_DISP_GET_INTF_TYPE: {
			enum sclr_vo_sel vo_sel;

			vo_sel = sclr_disp_mux_get();
			if (copy_to_user(ext_ctrls[i].ptr, &vo_sel, sizeof(vo_sel)))
				break;
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_DISP_GAMMA_LUT_UPDATE: {
			struct sclr_disp_gamma_attr gamma_attr;

			if (copy_from_user(&gamma_attr, (void *)ext_ctrls[i].ptr, sizeof(gamma_attr))) {
				dprintk(VIP_ERR, "gamma lut update copy_from_user failed.\n");
				break;
			}
			sclr_disp_gamma_ctrl(gamma_attr.enable, gamma_attr.pre_osd);
			sclr_disp_gamma_lut_update(gamma_attr.table, gamma_attr.table, gamma_attr.table);
			rc = 0;
		}
		break;

		case V4L2_CID_DV_VIP_DISP_GAMMA_LUT_READ: {
			struct sclr_disp_gamma_attr gamma_attr;

			sclr_disp_gamma_lut_read(&gamma_attr);
			if (copy_to_user((void *)ext_ctrls[i].ptr, &gamma_attr, sizeof(gamma_attr))) {
				dprintk(VIP_ERR, "gamma lut read copy_to_user failed.\n");
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

int cvi_disp_g_selection(struct file *file, void *priv,
		struct v4l2_selection *sel)
{
	struct cvi_disp_vdev *ddev = video_drvdata(file);
	int rc = -EINVAL;

	dprintk(VIP_DBG, "+\n");

	sel->r.left = sel->r.top = 0;
	switch (sel->target) {
	case V4L2_SEL_TGT_COMPOSE:
		sel->r = ddev->compose_out;
		rc = 0;
	break;

	case V4L2_SEL_TGT_COMPOSE_DEFAULT:
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
		sel->r = ddev->sink_rect;
		rc = 0;
	break;

	default:
		return rc;
	}

	dprintk(VIP_INFO, "target(%d) rect(%d %d %d %d)\n", sel->target,
			sel->r.left, sel->r.top, sel->r.width, sel->r.height);
	return rc;
}

int cvi_disp_s_selection(struct file *file, void *fh, struct v4l2_selection *sel)
{
	struct cvi_disp_vdev *ddev = video_drvdata(file);
	int rc = -EINVAL;

	dprintk(VIP_DBG, "+\n");

	switch (sel->target) {
	case V4L2_SEL_TGT_COMPOSE:
	if (memcmp(&ddev->compose_out, &sel->r, sizeof(sel->r))) {
		struct sclr_rect rect;

		vip_fill_rect_from_v4l2(rect, sel->r);
		if (sclr_disp_set_rect(rect) == 0)
			ddev->compose_out = sel->r;
	}
	rc = 0;
	break;

	case V4L2_SEL_TGT_CROP: {
		struct sclr_disp_cfg *cfg;

		cfg = sclr_disp_get_cfg();
		cfg->mem.start_x = sel->r.left;
		cfg->mem.start_y = sel->r.top;
		cfg->mem.width	 = sel->r.width;
		cfg->mem.height  = sel->r.height;
		sclr_disp_set_mem(&cfg->mem);
		ddev->crop_rect = sel->r;
		rc = 0;
	}
	break;

	default:
		return rc;
	}

	dprintk(VIP_INFO, "target(%d) rect(%d %d %d %d)\n", sel->target,
			sel->r.left, sel->r.top, sel->r.width, sel->r.height);
	return rc;
}

int cvi_disp_enum_fmt_vid_mplane(struct file *file, void  *priv,
		struct v4l2_fmtdesc *f)
{
	dprintk(VIP_DBG, "+\n");
	return cvi_vip_enum_fmt_vid(file, priv, f);
}

int cvi_disp_g_fmt_vid_out_mplane(struct file *file, void *priv,
		struct v4l2_format *f)
{
	struct cvi_disp_vdev *ddev = video_drvdata(file);
	struct v4l2_pix_format_mplane *mp = &f->fmt.pix_mp;
	unsigned int p;

	dprintk(VIP_DBG, "+\n");
	WARN_ON(!ddev);

	mp->width        = ddev->compose_out.width;
	mp->height       = ddev->compose_out.height;
	mp->field        = V4L2_FIELD_NONE;
	mp->pixelformat  = ddev->fmt->fourcc;
	mp->colorspace   = ddev->colorspace;
	mp->xfer_func    = V4L2_XFER_FUNC_DEFAULT;
	mp->ycbcr_enc    = V4L2_YCBCR_ENC_DEFAULT;
	mp->quantization = V4L2_QUANTIZATION_DEFAULT;
	mp->num_planes   = ddev->fmt->buffers;
	for (p = 0; p < mp->num_planes; p++) {
		mp->plane_fmt[p].bytesperline = ddev->bytesperline[p];
		mp->plane_fmt[p].sizeimage = ddev->sizeimage[p];
	}

	return 0;
}

int cvi_disp_try_fmt_vid_out_mplane(struct file *file, void *priv,
		struct v4l2_format *f)
{
	struct cvi_disp_vdev *ddev = video_drvdata(file);

	return cvi_vip_try_fmt_vid_mplane(f, ddev->align);
}

static void _fill_disp_cfg(struct sclr_disp_cfg *cfg,
		struct v4l2_pix_format_mplane *mp)
{
	const struct cvi_vip_fmt *fmt;
	struct v4l2_plane_pix_format *pfmt = mp->plane_fmt;

	fmt = cvi_vip_get_format(mp->pixelformat);

	cfg->fmt = fmt->fmt;
	if (mp->colorspace == V4L2_COLORSPACE_SRGB)
		cfg->in_csc = SCL_CSC_NONE;
	else if (mp->colorspace == V4L2_COLORSPACE_SMPTE170M)
		cfg->in_csc = SCL_CSC_601_LIMIT_YUV2RGB;
	else
		cfg->in_csc = SCL_CSC_709_LIMIT_YUV2RGB;

	cfg->mem.pitch_y = pfmt[0].bytesperline;
	cfg->mem.pitch_c = pfmt[1].bytesperline;
	cfg->mem.width = mp->width;
	cfg->mem.height = mp->height;
	cfg->mem.start_x = 0;
	cfg->mem.start_y = 0;
}

int cvi_disp_s_fmt_vid_out_mplane(struct file *file, void *priv,
	struct v4l2_format *f)
{
	struct cvi_disp_vdev *ddev = video_drvdata(file);
	struct v4l2_pix_format_mplane *mp = &f->fmt.pix_mp;
	struct v4l2_plane_pix_format *pfmt = mp->plane_fmt;
	const struct cvi_vip_fmt *fmt;
	unsigned int p;
	struct sclr_disp_cfg *cfg;
	int rc = cvi_disp_try_fmt_vid_out_mplane(file, priv, f);

	dprintk(VIP_DBG, "+\n");
	if (rc < 0)
		return rc;

	fmt = cvi_vip_get_format(mp->pixelformat);
	ddev->fmt = fmt;
	ddev->colorspace = mp->colorspace;
	for (p = 0; p < mp->num_planes; p++) {
		ddev->bytesperline[p] = pfmt[p].bytesperline;
		ddev->sizeimage[p] = pfmt[p].sizeimage;
	}

	cfg = sclr_disp_get_cfg();
	_fill_disp_cfg(cfg, mp);
	sclr_disp_set_cfg(cfg);
	return rc;
}

static void _fill_disp_timing(struct sclr_disp_timing *timing,
		struct v4l2_bt_timings *bt_timing)
{
	timing->vtotal = V4L2_DV_BT_FRAME_HEIGHT(bt_timing) - 1;
	timing->htotal = V4L2_DV_BT_FRAME_WIDTH(bt_timing) - 1;
	timing->vsync_start = 1;
	timing->vsync_end = timing->vsync_start + bt_timing->vsync - 1;
	timing->vfde_start = timing->vmde_start =
		timing->vsync_start + bt_timing->vsync + bt_timing->vbackporch;
	timing->vfde_end = timing->vmde_end =
		timing->vfde_start + bt_timing->height - 1;
	timing->hsync_start = 1;
	timing->hsync_end = timing->hsync_start + bt_timing->hsync - 1;
	timing->hfde_start = timing->hmde_start =
		timing->hsync_start + bt_timing->hsync + bt_timing->hbackporch;
	timing->hfde_end = timing->hmde_end =
		timing->hfde_start + bt_timing->width - 1;
	timing->vsync_pol = bt_timing->polarities & V4L2_DV_VSYNC_POS_POL;
	timing->hsync_pol = bt_timing->polarities & V4L2_DV_HSYNC_POS_POL;
}

int cvi_disp_s_dv_timings(struct file *file, void *_fh,
		struct v4l2_dv_timings *timings)
{
	struct cvi_disp_vdev *ddev = video_drvdata(file);
	struct sclr_disp_timing timing;

	dprintk(VIP_DBG, "+\n");
//	if (!v4l2_find_dv_timings_cap(timings, &cvi_disp_dv_timings_caps, 0,
//		NULL, NULL))
//		return -EINVAL;

//	if (v4l2_match_dv_timings(timings, &ddev->dv_timings, 0, false))
//		return 0;
	if (!list_empty(&ddev->rdy_queue))
		return -EBUSY;

	ddev->dv_timings = *timings;
	ddev->sink_rect.width = timings->bt.width;
	ddev->sink_rect.height = timings->bt.height;
	ddev->compose_out = ddev->sink_rect;
	dprintk(VIP_INFO, "timing %d-%d\n", timings->bt.width, timings->bt.height);

	_fill_disp_timing(&timing, &timings->bt);
	sclr_disp_set_timing(&timing);
	return 0;
}

int cvi_disp_g_dv_timings(struct file *file, void *_fh,
			struct v4l2_dv_timings *timings)
{
	struct cvi_disp_vdev *ddev = video_drvdata(file);

	dprintk(VIP_DBG, "+\n");
	*timings = ddev->dv_timings;
	return 0;
}

int cvi_disp_enum_dv_timings(struct file *file, void *_fh,
			struct v4l2_enum_dv_timings *timings)
{
	return v4l2_enum_dv_timings_cap(timings, &cvi_disp_dv_timings_caps,
			NULL, NULL);
}

int cvi_disp_dv_timings_cap(struct file *file, void *_fh,
			struct v4l2_dv_timings_cap *cap)
{
	*cap = cvi_disp_dv_timings_caps;
	return 0;
}

#if 0
int cvi_disp_streamon(struct file *file, void *fh, enum v4l2_buf_type i)
{
	struct cvi_disp_vdev *ddev = video_drvdata(file);
	int rc = 0;

	rc = vb2_streamon(&ddev->vb_q, i);
	if (!rc) {
		struct sclr_top_cfg *cfg = sclr_top_get_cfg();

		cfg->disp_enable = true;
		sclr_top_set_cfg(cfg);
	}
	return rc;
}

int cvi_disp_streamoff(struct file *file, void *fh, enum v4l2_buf_type i)
{
	struct cvi_disp_vdev *ddev = video_drvdata(file);
	struct sclr_top_cfg *cfg = sclr_top_get_cfg();
	int rc = 0;

	rc = vb2_streamoff(&ddev->vb_q, i);
	cfg->disp_enable = false;
	sclr_top_set_cfg(cfg);
	return rc;
}
#endif

static const struct v4l2_ioctl_ops cvi_disp_ioctl_ops = {
	.vidioc_querycap = cvi_disp_querycap,
	.vidioc_g_ctrl = cvi_disp_g_ctrl,
	.vidioc_s_ctrl = cvi_disp_s_ctrl,
	.vidioc_s_ext_ctrls = cvi_disp_s_ext_ctrls,

	.vidioc_g_selection     = cvi_disp_g_selection,
	.vidioc_s_selection     = cvi_disp_s_selection,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	.vidioc_enum_fmt_vid_out = cvi_disp_enum_fmt_vid_mplane,
#else
	.vidioc_enum_fmt_vid_out_mplane = cvi_disp_enum_fmt_vid_mplane,
#endif
	.vidioc_g_fmt_vid_out_mplane    = cvi_disp_g_fmt_vid_out_mplane,
	.vidioc_try_fmt_vid_out_mplane  = cvi_disp_try_fmt_vid_out_mplane,
	.vidioc_s_fmt_vid_out_mplane    = cvi_disp_s_fmt_vid_out_mplane,

	.vidioc_s_dv_timings        = cvi_disp_s_dv_timings,
	.vidioc_g_dv_timings        = cvi_disp_g_dv_timings,
//	.vidioc_query_dv_timings    = cvi_disp_query_dv_timings,
	.vidioc_enum_dv_timings     = cvi_disp_enum_dv_timings,
	.vidioc_dv_timings_cap      = cvi_disp_dv_timings_cap,

	.vidioc_reqbufs         = vb2_ioctl_reqbufs,
	//.vidioc_create_bufs     = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf     = vb2_ioctl_prepare_buf,
	.vidioc_querybuf        = vb2_ioctl_querybuf,
	.vidioc_qbuf            = vb2_ioctl_qbuf,
	.vidioc_dqbuf           = vb2_ioctl_dqbuf,
	//.vidioc_expbuf          = vb2_ioctl_expbuf,
	.vidioc_streamon        = vb2_ioctl_streamon,
	.vidioc_streamoff       = vb2_ioctl_streamoff,

	.vidioc_subscribe_event     = disp_subscribe_event,
	.vidioc_unsubscribe_event   = v4l2_event_unsubscribe,
};

/*************************************************************************
 * proc functions
 *************************************************************************/
static void _show_mem(struct seq_file *m, struct sclr_mem *mem)
{
	seq_printf(m, "start_x(%3d)\t\tstart_y(%3d)\t\twidth(%4d)\t\theight(%4d)\n"
		  , mem->start_x, mem->start_y, mem->width, mem->height);
	seq_printf(m, "pitch_y(%3d)\t\tpitch_c(%3d)\n", mem->pitch_y, mem->pitch_c);
}

static void _show_disp_status(struct seq_file *m)
{
	struct sclr_disp_cfg *cfg = sclr_disp_get_cfg();
	union sclr_disp_dbg_status status = sclr_disp_get_dbg_status(true);
	struct sclr_disp_timing *timing = sclr_disp_get_timing();

	seq_puts(m, "--------------DISP----------------------------\n");
	seq_printf(m, "disp_from_sc(%d)\t\tsync_ext(%d)\t\ttgen_en(%d)\t\tfmt(%s)\n"
		  , cfg->disp_from_sc, cfg->sync_ext, cfg->tgen_en, str_sclr_fmt[cfg->fmt]);
	seq_printf(m, "in_csc(%15s)\tout_csc(%15s)burst(%d)\n"
		  , str_sclr_csc[cfg->in_csc], str_sclr_csc[cfg->out_csc], cfg->burst);
	_show_mem(m, &cfg->mem);
	seq_printf(m, "err_fwr_yuv(%d%d%d)\terr_erd_yuv(%d%d%d)\tlb_full_yuv(%d%d%d)\tlb_empty_yuv(%d%d%d)\n"
		  , status.b.err_fwr_y, status.b.err_fwr_u,  status.b.err_fwr_v, status.b.err_erd_y
		  , status.b.err_erd_u, status.b.err_erd_v, status.b.lb_full_y, status.b.lb_full_u
		  , status.b.lb_full_v, status.b.lb_empty_y, status.b.lb_empty_u, status.b.lb_empty_v);
	seq_printf(m, "bw fail(%d)\n", status.b.bw_fail);
	seq_puts(m, "--------------DISP-TIMING---------------------\n");
	seq_printf(m, "total(%4d * %4d)\thsync_pol(%4d)\tvsync_pol(%4d)\n"
		  , timing->htotal, timing->vtotal, timing->hsync_pol, timing->vsync_pol);
	seq_printf(m, "hsync_start(%4d)\thsync_end(%4d)\tvsync_start(%4d)\tvsync_end(%4d)\n"
		  , timing->htotal, timing->vtotal, timing->vsync_start, timing->vsync_end);
	seq_printf(m, "hde-start(%4d)\t\thde-end(%4d)\tvde-start(%4d)\t\tvde-end(%4d)\t\n"
		  , timing->hfde_start, timing->hfde_end, timing->vfde_start, timing->vfde_end);
}

static int disp_proc_show(struct seq_file *m, void *v)
{
	// show driver status if vpss_mode == 1
	if (proc_vo_mode) {
		_show_disp_status(m);
		return 0;
	}

	vo_proc_show(m, v);
	return 0;
}

static int disp_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, disp_proc_show, PDE_DATA(inode));
}

static ssize_t disp_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
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

	if (kstrtoint(cProcInputdata, 10, &proc_vo_mode))
		proc_vo_mode = 0;

	return count;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops disp_proc_fops = {
	.proc_open = disp_proc_open,
	.proc_read = seq_read,
	.proc_write = disp_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};
#else
static const struct file_operations disp_proc_fops = {
	.owner = THIS_MODULE,
	.open = disp_proc_open,
	.read = seq_read,
	.write = disp_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

/*************************************************************************
 *	General functions
 *************************************************************************/
int disp_create_instance(struct platform_device *pdev)
{
	int rc = 0;
	struct cvi_vip_dev *bdev;
	struct video_device *vfd;
	struct cvi_disp_vdev *ddev;
	struct vb2_queue *q;
	u16 rgb[3] = {0, 0, 0};

	g_pdev = pdev;
	bdev = dev_get_drvdata(&pdev->dev);
	if (!bdev) {
		dprintk(VIP_ERR, "invalid data\n");
		return -EINVAL;
	}
	ddev = &bdev->disp_vdev;
	mutex_init(&ddev->mutex);

	ddev->clk_disp = devm_clk_get(&pdev->dev, CLK_DISP_NAME);
	if (IS_ERR(ddev->clk_disp)) {
		pr_err("Cannot get clk for disp\n");
		ddev->clk_disp = NULL;
	}
	ddev->clk_bt = devm_clk_get(&pdev->dev, CLK_BT_NAME);
	if (IS_ERR(ddev->clk_bt)) {
		pr_err("Cannot get clk for bt\n");
		ddev->clk_bt = NULL;
	}
	ddev->clk_dsi = devm_clk_get(&pdev->dev, CLK_DSI_NAME);
	if (IS_ERR(ddev->clk_dsi)) {
		pr_err("Cannot get clk for dsi\n");
		ddev->clk_dsi = NULL;
	}

	if (sclr_disp_mux_get() == SCLR_VO_SEL_I80) {
		smooth = sclr_disp_check_i80_enable();
	} else {
		smooth = sclr_disp_check_tgen_enable();
	}
	if (!smooth) {
		if (ddev->clk_disp && __clk_is_enabled(ddev->clk_disp))
			clk_disable_unprepare(ddev->clk_disp);
		if (ddev->clk_bt && __clk_is_enabled(ddev->clk_bt))
			clk_disable_unprepare(ddev->clk_bt);
		if (ddev->clk_dsi && __clk_is_enabled(ddev->clk_dsi))
			clk_disable_unprepare(ddev->clk_dsi);
	}

	ddev->align = VIP_ALIGNMENT;
	ddev->fmt = cvi_vip_get_format(V4L2_PIX_FMT_RGBM);
	ddev->vid_caps = V4L2_CAP_VIDEO_OUTPUT_MPLANE | V4L2_CAP_STREAMING;
	ddev->dv_timings = def_dv_timings[0];
	memset(&ddev->sink_rect, 0, sizeof(ddev->sink_rect));
	ddev->sink_rect.width = ddev->dv_timings.bt.width;
	ddev->sink_rect.height = ddev->dv_timings.bt.height;
	ddev->bgcolor_enable = false;

	vfd = &(ddev->vdev);
	snprintf(vfd->name, sizeof(vfd->name), "cvi-disp");
	vfd->fops = &cvi_disp_fops;
	vfd->ioctl_ops = &cvi_disp_ioctl_ops;
	vfd->vfl_dir = VFL_DIR_TX;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	vfd->vfl_type = VFL_TYPE_VIDEO;
#else
	vfd->vfl_type = VFL_TYPE_GRABBER;
#endif
	vfd->minor = -1;
	vfd->device_caps = ddev->vid_caps;
	vfd->release = video_device_release_empty;
	vfd->v4l2_dev = &bdev->v4l2_dev;
	vfd->lock = &ddev->mutex;
	vfd->queue = &ddev->vb_q;

	// vb2_queue init
	q = &ddev->vb_q;
	q->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	q->io_modes = VB2_USERPTR;
	q->buf_struct_size = sizeof(struct cvi_vip_buffer);
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->min_buffers_needed = 0;
	q->drv_priv = ddev;
	q->dev = bdev->v4l2_dev.dev;
	q->ops = &cvi_disp_qops;
	q->mem_ops = &cvi_disp_vb2_mem_ops;
	//q->lock = &ddev->mutex;
	rc = vb2_queue_init(q);
	if (rc) {
		dprintk(VIP_ERR, "vb2_queue_init failed, ret=%d\n", rc);
		return rc;
	}
	spin_lock_init(&ddev->rdy_lock);
	INIT_LIST_HEAD(&ddev->rdy_queue);
	ddev->num_rdy = 0;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	rc = video_register_device(vfd, VFL_TYPE_VIDEO, DISP_DEVICE_IDX);
#else
	rc = video_register_device(vfd, VFL_TYPE_GRABBER, DISP_DEVICE_IDX);
#endif
	if (rc) {
		dprintk(VIP_ERR, "Failed to register %s, ret=%d\n", vfd->name, rc);
		return rc;
	}

	video_set_drvdata(vfd, ddev);

	dprintk(VIP_INFO, "disp registered as %s\n",
			video_device_node_name(vfd));

	ddev->shared_mem = kzalloc(VO_SHARE_MEM_SIZE, GFP_ATOMIC);
	if (!ddev->shared_mem) {
		dprintk(VIP_ERR, "vo shared mem alloc fail\n");
		return -ENOMEM;
	}

	if (vo_proc_init(ddev->shared_mem) < 0)
		dprintk(VIP_ERR, "vo proc init failed\n");

	if (proc_create_data(DISP_PROC_NAME, 0644, NULL, &disp_proc_fops, &bdev->disp_vdev) == NULL)
		dprintk(VIP_ERR, "disp proc creation failed\n");

	if (hide_vo) {
		sclr_disp_set_pattern(SCL_PAT_TYPE_FULL, SCL_PAT_COLOR_USR, rgb);
		sclr_disp_set_frame_bgcolor(0, 0, 0);
	}

	return rc;
}

int disp_destroy_instance(struct platform_device *pdev)
{
	struct cvi_vip_dev *bdev;
	struct cvi_disp_vdev *ddev;

	bdev = dev_get_drvdata(&pdev->dev);
	if (!bdev) {
		dprintk(VIP_ERR, "invalid data\n");
		return -EINVAL;
	}

	remove_proc_entry(DISP_PROC_NAME, NULL);
	vo_proc_remove();
	ddev = &bdev->disp_vdev;
	kfree(ddev->shared_mem);

	return 0;
}

void disp_irq_handler(union sclr_intr intr_status, struct cvi_vip_dev *bdev)
{
	if (!vb2_is_streaming(&bdev->disp_vdev.vb_q))
		return;

	if (intr_status.b.disp_frame_end) {
		struct cvi_disp_vdev *ddev = &bdev->disp_vdev;
		struct cvi_vip_buffer *b = NULL;
		union sclr_disp_dbg_status status = sclr_disp_get_dbg_status(true);

		++bdev->disp_vdev.frame_number;

		if (status.b.bw_fail)
			dprintk(VIP_ERR, " disp bw failed at frame#%d\n", bdev->disp_vdev.frame_number);
		if (status.b.osd_bw_fail)
			dprintk(VIP_ERR, " osd bw failed at frame#%d\n", bdev->disp_vdev.frame_number);

		// i80 won't need to keep one frame for read, but others need.
		if ((ddev->num_rdy > 1) || (ddev->disp_interface == CVI_VIP_DISP_INTF_I80)) {
			b = cvi_vip_buf_remove((struct cvi_base_vdev *)ddev);
			if (!b)
				return;
			// muted until frame available.
			if (!ddev->bgcolor_enable)
				sclr_disp_enable_window_bgcolor(false);
			b->vb.vb2_buf.timestamp = ktime_get_ns();
			b->vb.sequence = ++ddev->seq_count;
			vb2_buffer_done(&b->vb.vb2_buf, VB2_BUF_STATE_DONE);

			_hw_enque(ddev);
		}

	}
}
