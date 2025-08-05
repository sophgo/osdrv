#include "cif_ioctl.h"
#include "cif_comm.h"
#include "cif.h"
#include "io.h"
#include <string.h>
#include <errno.h>
#include "osal_ioctl.h"

extern struct cif_dev *g_cif_pdev;

static long _cif_ioctl(struct cif_dev *dev, unsigned int cmd,
		       unsigned long arg)
{
	struct cif_ctx *ctx = NULL;
	uint32_t devno;

	TRACE_CIF(DBG_DEBUG, "%s\n", _to_string_cmd(cmd));

	if (arg == 0) {
		TRACE_CIF(DBG_ERR, "null pointer\n");
		return -EINVAL;
	}

	switch (cmd) {
	case MIPI_SET_DEV_ATTR:
	{
		struct combo_dev_attr_s attr;

		osal_memcpy(&attr, (void *)arg, sizeof(attr));

		return cif_set_dev_attr(dev, &attr);
	}
	case MIPI_SET_OUTPUT_CLK_EDGE:
	{
		struct clk_edge_s clk;

		osal_memcpy(&clk, (void *)arg, sizeof(clk));

		return cif_set_output_clk_edge(dev, &clk);
	}
	case MIPI_RESET_MIPI:

		devno = *(uint32_t *)arg;

		return cif_reset_mipi(dev, devno);
	case MIPI_SET_CROP_TOP: // remove info line
	{
		struct crop_top_s crop;

		osal_memcpy(&crop, (void *)arg, sizeof(crop));

		return cif_set_crop_top(dev, &crop);
	}
	case MIPI_SET_CROP_WINDOW: // crop input image
	{
		struct cif_crop_win_s win;

		osal_memcpy(&win, (void *)arg, sizeof(win));

		return cif_set_windowing(dev, &win);
	}
	case MIPI_SET_WDR_MANUAL:
	{
		struct manual_wdr_s wdr_manu;

		osal_memcpy(&wdr_manu, (void *)arg, sizeof(wdr_manu));

		return cif_set_wdr_manual(dev, &wdr_manu);
	}
	case MIPI_SET_LVDS_FP_VS:
	{
		struct vsync_gen_s vsync;

		osal_memcpy(&vsync, (void *)arg, sizeof(vsync));

		return cif_set_lvds_fp_vs(dev, &vsync);
	}
	case MIPI_RESET_SENSOR:
	{
		sns_rst_config rst_config;

		osal_memcpy(&rst_config, (void *)arg, sizeof(rst_config));

		return cif_reset_snsr_gpio(dev, &rst_config, 1);
	}
	case MIPI_UNRESET_SENSOR:
	{
		sns_rst_config rst_config;

		osal_memcpy(&rst_config, (void *)arg, sizeof(rst_config));

		return cif_reset_snsr_gpio(dev, &rst_config, 0);
	}
	case MIPI_ENABLE_SENSOR_CLOCK:
	{
		devno = *(uint32_t *)arg;

		return cif_enable_snsr_clk(dev, devno, 1);
	}
	case MIPI_DISABLE_SENSOR_CLOCK:
	{
		devno = *(uint32_t *)arg;

		return cif_enable_snsr_clk(dev, devno, 0);
	}
	case MIPI_RESET_LVDS:
	case CIF_CB_RESET_LVDS:
	{
		devno = *(uint32_t *)arg;

		return cif_reset_lvds(dev, devno);
	}
	case MIPI_GET_CIF_ATTR:
	case CIF_CB_GET_CIF_ATTR:
	{
		struct cif_attr_s cif_attr;

		osal_memcpy(&cif_attr, (void *)arg, sizeof(cif_attr));

		cif_get_cif_attr(dev, &cif_attr);

		osal_memcpy((void *)arg, &cif_attr, sizeof(cif_attr));

		return 0;
	}
	case MIPI_SET_BT_FMT_OUT:
	{
		struct bt_fmt_out_s bt_fmt;

		osal_memcpy(&bt_fmt, (void *)arg, sizeof(bt_fmt));

		return cif_bt_fmt_out(dev, &bt_fmt);
	}
	case MIPI_SET_SENSOR_CLOCK:
	{
		struct mclk_pll_s mclk;

		osal_memcpy(&mclk, (void *)arg, sizeof(mclk));

		/* sensor enable */
		if (mclk.cam >= MAX_LINK_NUM)
			return -EINVAL;
		if (mclk.cam < MAX_CAM_CLK_NUM) {
			snsr_mclk[mclk.cam] = mclk.freq;
		} else {
			TRACE_CIF(DBG_ERR, "cam clk out of max clk list.\n");
		}

		return _cif_enable_snsr_clk(dev, mclk.cam, mclk.freq != CAMPLL_FREQ_NONE);
	}
	case MIPI_SET_MAX_MAC_CLOCK:
	{

		dev->max_mac_clk = *(uint32_t *)arg;

		if (dev->max_mac_clk <= 400)
			dev->max_mac_clk = 396;
		else if (dev->max_mac_clk <= 500)
			dev->max_mac_clk = 500;
		else
			dev->max_mac_clk = 594;

		break;
	}
	case MIPI_SET_YUV_SWAP:
	{
		struct cif_yuv_swap_s swap;

		osal_memcpy(&swap, (void *)arg, sizeof(swap));

		ctx = &dev->link[swap.devno].cif_ctx;

		return cif_swap_yuv(ctx, swap.uv_swap, swap.yc_swap);
	}
	default:
		return -OSAL_ENOIOCTLCMD;
	}
	return 0;
}

long cif_ioctl(unsigned int cmd, unsigned long arg)
{
	struct cif_dev *dev = g_cif_pdev;

	return _cif_ioctl(dev, cmd, arg);
}
