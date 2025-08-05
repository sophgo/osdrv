#include <aos/kernel.h>
#include "osal.h"
#include "vo_platform.h"
#include "vo_process.h"
#include "vo_debug.h"
#include "vo_sdk_layer.h"
#include "pinctrl-mars.h"

int vo_log_lv = DBG_WARN;
int hide_vo;

static osal_atomic dev_open_cnt;
//to do: smooth from uboot
int smooth[VO_MAX_DEV_NUM];

#define C906_DISP_INT_NUM (10 + 16)

//update proc info
static aos_timer_t vo_proc_timer;
static void vo_updatel_frame_rate(void *timer, void *arg)
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
}

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

int vo_create_instance()
{
	struct vbq_recv_s recv_vb = {vo_recv_frame, NULL};
	struct vo_ctx *ctx = NULL;

	ctx = osal_malloc(sizeof(struct vo_ctx));
	if (!ctx) {
		TRACE_VO(DBG_ERR, "fail to osal_malloc!\n");
		return OSAL_ENOMEM;
	}

	g_vo_ctx = ctx;

	osal_memset(ctx, 0, sizeof(struct vo_ctx));

	g_vo_ctx->dev_ctx[0].irq_num = C906_DISP_INT_NUM;

	disp_ctrl_init(false);

	vo_process_init_ctx(ctx);

	base_register_recv_cb(ID_VO, &recv_vb);

	aos_timer_new(&vo_proc_timer, vo_updatel_frame_rate, NULL, 1000, AOS_TIMER_REPEAT);
	aos_timer_start(&vo_proc_timer);

	return 0;
}

int vo_destroy_instance()
{
	struct vo_ctx *ctx = g_vo_ctx;

	disp_ctrl_deinit();
	vo_process_deinit_ctx(ctx);
	if (aos_timer_is_valid(&vo_proc_timer)) {
		aos_timer_stop(&vo_proc_timer);
		aos_timer_free(&vo_proc_timer);
	}
	base_unregister_recv_cb(ID_VO);
	osal_free(ctx);

	return 0;
}

static long _vo_s_ctrl(struct vo_ext_control *p)
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

static long _vo_g_ctrl(struct vo_ext_control *p)
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

long driver_vo_ioctl(unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct vo_ext_control p;

	CHECK_IOCTL_CMD(cmd, struct vo_ext_control);

	if (osal_copy_from_user(&p, (void *)arg, sizeof(struct vo_ext_control)))
		return -14;

	switch (cmd) {
	case VO_IOC_G_CTRL:
	{
		ret = _vo_g_ctrl(&p);
		break;
	}
	case VO_IOC_S_CTRL:
	{
		ret = _vo_s_ctrl(&p);
		break;
	}
	default:
		ret = -25;
		break;
	}

	if (osal_copy_to_user((void *)arg, &p, sizeof(struct vo_ext_control)))
		return -14;

	return ret;
}

int vo_open()
{
	int ret = 0;
	vo_dev dev;

	if (!osal_atomic_read(&dev_open_cnt)) {
		for (dev = 0; dev < VO_MAX_DEV_NUM; ++dev) {
			osal_atomic_set(&g_vo_ctx->dev_ctx[dev].disp_streamon, 0);
			disp_reg_shadow_sel(dev, false);
			// if (!smooth[dev])
			disp_set_cfg(dev, disp_get_cfg(dev));
		}
	}

	osal_atomic_inc(&dev_open_cnt);

	return ret;
}

void vo_sdk_release()
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

int vo_release()
{
	int ret = 0;
	osal_atomic_dec(&dev_open_cnt);
	if (!osal_atomic_read(&dev_open_cnt)) {
		vo_sdk_release();
	}
	return ret;
}
