#include <linux/poll.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include "pinctrl-cv184x.h"
#include "proc/vo_proc.h"
#include "proc/vo_disp_proc.h"
#include "vo_drv.h"
#include "vo_platform.h"
#include "vo_process.h"
#include "vo_debug.h"
#include "vo_sdk_layer.h"

int vo_log_lv = DBG_WARN;
int hide_vo;
module_param(vo_log_lv, int, 0644);
module_param(hide_vo, int, 0444);

static atomic_t dev_open_cnt;
//to do: smooth from uboot
int smooth[VO_MAX_DEV_NUM];

extern const char *const disp_irq_name[DISP_MAX_INST];

//update proc info
void vo_updatel_frame_rate(struct timer_list *timer);
DEFINE_TIMER(vo_timer_proc, vo_updatel_frame_rate);

void vo_config_pinmux(unsigned int pad)
{
	switch (pad) {
		case VO_MIPI_TXP2:
			PINMUX_CONFIG(PAD_MIPI_TXP2, VO_CLK0);
		break;
		case VO_VIVO_CLK:
			PINMUX_CONFIG(VIVO_CLK, VO_CLK1);
		break;
		case VO_MIPI_TXM2:
			PINMUX_CONFIG(PAD_MIPI_TXM2, VO_D_0);
		break;
		case VO_MIPI_TXP1:
			PINMUX_CONFIG(PAD_MIPI_TXP1, VO_D_1);
		break;
		case VO_MIPI_TXM1:
			PINMUX_CONFIG(PAD_MIPI_TXM1, VO_D_2);
		break;
		case VO_MIPI_TXP0:
			PINMUX_CONFIG(PAD_MIPI_TXP0, VO_D_3);
		break;
		case VO_MIPI_TXM0:
			PINMUX_CONFIG(PAD_MIPI_TXM0, VO_D_4);
		break;
		case VO_MIPI_RXP0:
			PINMUX_CONFIG(PAD_MIPIRX0P, VO_D_5);
		break;
		case VO_MIPI_RXN0:
			PINMUX_CONFIG(PAD_MIPIRX0N, VO_D_6);
		break;
		case VO_MIPI_RXP1:
			PINMUX_CONFIG(PAD_MIPIRX1P, VO_D_7);
		break;
		case VO_MIPI_RXN1:
			PINMUX_CONFIG(PAD_MIPIRX1N, VO_D_8);
		break;
		case VO_MIPI_RXP2:
			PINMUX_CONFIG(PAD_MIPIRX2P, VO_D_9);
		break;
		case VO_MIPI_RXN2:
			PINMUX_CONFIG(PAD_MIPIRX2N, VO_D_10);
		break;
		case VO_MIPI_RXP5:
			PINMUX_CONFIG(PAD_MIPIRX5P, VO_D_11);
		break;
		case VO_MIPI_RXN5:
			PINMUX_CONFIG(PAD_MIPIRX5N, VO_D_12);
		break;
		case VO_VIVO_D0:
			PINMUX_CONFIG(VIVO_D0, VO_D_13);
		break;
		case VO_VIVO_D1:
			PINMUX_CONFIG(VIVO_D1, VO_D_14);
		break;
		case VO_VIVO_D2:
			PINMUX_CONFIG(VIVO_D2, VO_D_15);
		break;
		case VO_VIVO_D3:
			PINMUX_CONFIG(VIVO_D3, VO_D_16);
		break;
		case VO_VIVO_D4:
			PINMUX_CONFIG(VIVO_D4, VO_D_17);
		break;
		case VO_VIVO_D5:
			PINMUX_CONFIG(VIVO_D5, VO_D_18);
		break;
		case VO_VIVO_D6:
			PINMUX_CONFIG(VIVO_D6, VO_D_19);
		break;
		case VO_VIVO_D7:
			PINMUX_CONFIG(VIVO_D7, VO_D_20);
		break;
		case VO_VIVO_D8:
			PINMUX_CONFIG(VIVO_D8, VO_D_21);
		break;
		case VO_VIVO_D9:
			PINMUX_CONFIG(VIVO_D9, VO_D_22);
		break;
		case VO_VIVO_D10:
			PINMUX_CONFIG(VIVO_D10, VO_D_23);
		break;
		case VO_MIPI_TXM4:
			PINMUX_CONFIG(PAD_MIPI_TXM4, VO_D_24);
		break;
		case VO_MIPI_TXP4:
			PINMUX_CONFIG(PAD_MIPI_TXP4, VO_D_25);
		break;
		case VO_MIPI_TXM3:
			PINMUX_CONFIG(PAD_MIPI_TXM3, VO_D_26);
		break;
		case VO_MIPI_TXP3:
			PINMUX_CONFIG(PAD_MIPI_TXP3, VO_D_27);
		break;
		case VO_JTAG_CPU_TMS:
			PINMUX_CONFIG(JTAG_CPU_TMS, VO_D_28);
		break;
		case VO_JTAG_CPU_TCK:
			PINMUX_CONFIG(JTAG_CPU_TCK, VO_D_29);
		break;
		case VO_JTAG_CPU_TRST:
			PINMUX_CONFIG(JTAG_CPU_TRST, VO_D_30);
		break;
		case VO_AUX0:
			PINMUX_CONFIG(AUX0, VO_D_31);
		break;
		case VO_SD1_D3:
			PINMUX_CONFIG(SD1_D3, PWR_SD1_D3_VO32);
		break;
		case VO_SD1_D2:
			PINMUX_CONFIG(SD1_D2, PWR_SD1_D2_VO33);
		break;
		case VO_SD1_D1:
			PINMUX_CONFIG(SD1_D1, PWR_SD1_D1_VO34);
		break;
		case VO_SD1_D0:
			PINMUX_CONFIG(SD1_D0, PWR_SD1_D0_VO35);
		break;
		case VO_SD1_CMD:
			PINMUX_CONFIG(SD1_CMD, PWR_SD1_CMD_VO36);
		break;
		case VO_SD1_CLK:
			PINMUX_CONFIG(SD1_CLK, PWR_SD1_CLK_VO37);
		break;
		default:
		break;
		}
}

void vo_updatel_frame_rate(struct timer_list *timer)
{
	int i, j;

	(void)timer;

	for (i = 0; i < VO_MAX_VIDEO_LAYER_NUM; ++i) {
		if (!g_vo_ctx->layer_ctx[i].is_layer_enable)
			continue;
		g_vo_ctx->layer_ctx[i].frame_rate = g_vo_ctx->layer_ctx[i].frame_num;
		g_vo_ctx->layer_ctx[i].frame_num = 0;

		g_vo_ctx->layer_ctx[i].src_frame_rate = g_vo_ctx->layer_ctx[i].src_frame_num;
		g_vo_ctx->layer_ctx[i].src_frame_num = 0;

		for (j = 0; j < VO_MAX_CHN_NUM; ++j) {
			if (!g_vo_ctx->layer_ctx[i].chn_ctx[j].is_chn_enable)
				continue;
			g_vo_ctx->layer_ctx[i].chn_ctx[j].frame_rate =
				g_vo_ctx->layer_ctx[i].chn_ctx[j].frame_num;
			g_vo_ctx->layer_ctx[i].chn_ctx[j].frame_num = 0;

			g_vo_ctx->layer_ctx[i].chn_ctx[j].src_frame_rate =
				g_vo_ctx->layer_ctx[i].chn_ctx[j].src_frame_num;
			g_vo_ctx->layer_ctx[i].chn_ctx[j].src_frame_num = 0;
		}
	}

	mod_timer(&vo_timer_proc, jiffies + msecs_to_jiffies(1000));
}

int vo_platform_create_proc(struct vo_ctx *ctx)
{
	int ret = 0;

	if (vo_proc_init(ctx) < 0) {
		TRACE_VO(DBG_ERR, "vo proc init failed\n");
		return -EAGAIN;
	}

	if (vo_disp_proc_init() < 0) {
		TRACE_VO(DBG_ERR, "proc init failed\n");
		return -EAGAIN;
	}

	return ret;
}

void vo_platform_destroy_proc(void)
{
	vo_disp_proc_remove();
	vo_proc_remove();
}

int vo_create_instance(struct platform_device *pdev)
{
	int ret = 0;
	int i = 0;
	struct vo_dev_ctx *dev_ctx;
	struct vbq_recv_s recv_vb = {vo_recv_frame, NULL};

	g_vo_ctx = kzalloc(sizeof(*g_vo_ctx), GFP_ATOMIC);
	if (!g_vo_ctx) {
		TRACE_VO(DBG_ERR, "g_vo_ctx alloc size(%zu) failed\n", sizeof(struct vo_ctx));
		return -ENOMEM;
	}

	disp_ctrl_init(false);

	vo_process_init_ctx(g_vo_ctx);

	for (i = 0; i < DISP_MAX_INST; ++i) {
		dev_ctx = &g_vo_ctx->dev_ctx[i];
		/* Interrupts */
		dev_ctx->irq_num = platform_get_irq_byname(pdev, disp_irq_name[i]);
		if (dev_ctx->irq_num < 0) {
			dev_err(&pdev->dev, "No IRQ resource for %s\n",  disp_irq_name[i]);
			return -ENOENT;
		}
		dev_info(&pdev->dev, "irq(%d) for %s get from platform driver.\n",
				dev_ctx->irq_num,  disp_irq_name[i]);
	}

	ret = vo_platform_create_proc(g_vo_ctx);
	if (ret) {
		TRACE_VO(DBG_ERR, "Failed to create proc\n");
		return ret;
	}

	base_register_recv_cb(ID_VO, &recv_vb);

	add_timer(&vo_timer_proc);
	mod_timer(&vo_timer_proc, jiffies + msecs_to_jiffies(1000));

	return 0;
}

int vo_destroy_instance(struct platform_device *pdev)
{
	struct vo_core_dev *core_dev = dev_get_drvdata(&pdev->dev);
	if (!core_dev) {
		TRACE_VO(DBG_ERR, "VO has been destroyed!\n");
		return -EINVAL;
	}

	disp_ctrl_deinit();
	vo_process_deinit_ctx(g_vo_ctx);
	del_timer_sync(&vo_timer_proc);
	base_unregister_recv_cb(ID_VO);
	vo_platform_destroy_proc();
	kfree(g_vo_ctx);
	g_vo_ctx = NULL;
	core_dev = NULL;

	return 0;
}

static long _vo_s_ctrl(struct vo_core_dev *vdev, struct vo_ext_control *p)
{
	unsigned int id = p->id;
	long rc = 0;

	switch (id) {
	case VO_IOCTL_SDK_CTRL: {
		rc = vo_sdk_ctrl(p);
	}
	break;

	default:
		rc = vo_custom_ctrl(p);
		break;
	}

	return rc;
}

static long _vo_g_ctrl(struct vo_core_dev *vdev, struct vo_ext_control *p)
{
	unsigned int id = p->id;
	long rc = 0;

	switch (id) {

	default:
		rc = vo_custom_ctrl(p);
		break;
	}

	return rc;
}

long vo_ioctl(struct file *file, u_int cmd, u_long arg)
{
	struct vo_core_dev *vdev = file->private_data;
	int ret = 0;
	struct vo_ext_control p;

	CHECK_IOCTL_CMD(cmd, struct vo_ext_control);

	if (copy_from_user(&p, (void __user *)arg, sizeof(struct vo_ext_control)))
		return -EFAULT;

	switch (cmd) {
	case VO_IOC_G_CTRL:
	{
		ret = _vo_g_ctrl(vdev, &p);
		break;
	}
	case VO_IOC_S_CTRL:
	{
		ret = _vo_s_ctrl(vdev, &p);
		break;
	}
	default:
		ret = -ENOTTY;
		break;
	}

	if (copy_to_user((void __user *)arg, &p, sizeof(struct vo_ext_control)))
		return -EFAULT;

	return ret;
}

int vo_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	vo_dev dev;
	struct vo_dev_ctx *dev_ctx;
	struct vo_core_dev *vdev;

	vdev = container_of(inode->i_cdev, struct vo_core_dev, cdev);
	file->private_data = vdev;
	if (!atomic_read(&dev_open_cnt)) {
		for (dev = 0; dev < VO_MAX_DEV_NUM; ++dev) {
			dev_ctx = &g_vo_ctx->dev_ctx[dev];
			osal_atomic_set(&dev_ctx->disp_streamon, 0);
			disp_reg_shadow_sel(dev, false);
			// if (!smooth[dev])
			disp_set_cfg(dev, disp_get_cfg(dev));
		}
	}

	atomic_inc(&dev_open_cnt);

	return ret;
}

void vo_sdk_release(struct vo_core_dev *vdev)
{
	int i, j;

	for (i = 0; i < VO_MAX_VIDEO_LAYER_NUM; i++) {
		if (g_vo_ctx->layer_ctx[i].is_layer_enable) {
			for (j = 0; j < VO_MAX_CHN_NUM; j++) {
				if (g_vo_ctx->layer_ctx[i].chn_ctx[j].is_chn_enable)
					vo_disable_chn(i, j);
			}
			vo_disablevideolayer(i);
		}
	}

	for (i = 0; i < VO_MAX_DEV_NUM; i++) {
		if (g_vo_ctx->dev_ctx[i].is_dev_enable)
			vo_disable(i);
	}
}

int vo_release(struct inode *inode, struct file *file)
{
	int ret = 0;

	atomic_dec(&dev_open_cnt);
	if (!atomic_read(&dev_open_cnt)) {
		struct vo_core_dev *vdev;

		vdev = container_of(inode->i_cdev, struct vo_core_dev, cdev);
		vo_sdk_release(vdev);
	}
	return ret;
}

unsigned int vo_poll(struct file *file, struct poll_table_struct *wait)
{
	return 0;
}
